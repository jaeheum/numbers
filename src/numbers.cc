/*
Copyright 2020 Jaeheum Han

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
// cache/memory access latency measurement,
// create_random_chain() and chase_pointers() are from
// https://github.com/afborchert/pointer-chasing
// with a few adjustments:
// - `std::shuffle` instead of manual shuffling
// - lambda
//
// https://github.com/afborchert/pointer-chasing
// comes with the following copyright and license:
/*
   Copyright (c) 2016, 2018 Andreas F. Borchert
   All rights reserved.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
   KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
   WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/
#include "argh.h"
#include "nanobench.h"

#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#define S(x) (#x)

namespace fs = std::filesystem;
static const fs::path p = fs::current_path() / "deleteme";
// XXX __attribute__ strictly speaking, not portable
static void deleteme(__attribute__((unused)) int signal) {
  if (fs::exists(p)) {
    fs::remove(p);
  }
}
static void deleteme_atexit() {
  deleteme(0);
}

static constexpr size_t KiB = 1 << 10;
static constexpr size_t MiB = KiB * KiB;

// XXX use libcpuid to get the cache size data instead of relying on glibc?
// https://www.gnu.org/software/libc/manual/html_mono/libc.html#Constants-for-Sysconf
static const size_t PAGESIZE = sysconf(_SC_PAGESIZE);
static const size_t L1_cache_size = sysconf(_SC_LEVEL1_DCACHE_SIZE);
static const size_t cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
static const size_t L2_cache_size = sysconf(_SC_LEVEL2_CACHE_SIZE);
static const size_t L3_cache_size = sysconf(_SC_LEVEL3_CACHE_SIZE);
// XXX is 1GiB enough on Zen2 EPYC and such "large" cpus?
static const size_t multiples_of_L3 =
    std::clamp(sysconf(_SC_LEVEL3_CACHE_SIZE) *
                   static_cast<size_t>(std::thread::hardware_concurrency()),
               256 * MiB,
               1024 * MiB);

static std::mutex m;
static int mi = 0;

int main(int, char* argv[]) {
  std::signal(SIGINT, deleteme);
  std::atexit(deleteme_atexit);

  std::ostream* outstream = &std::cout;
 
  argh::parser cmdline(argv);
  if (cmdline[{"-c", "--concise"}]) {
    // print nanobench table output by setting bench.output(outstream)
    outstream = nullptr;
  }
  if (cmdline[{"-h", "--help"}]) {
    // XXX reorg the code as a table of Bench/{code, param, docstring}?
    ::printf("numbers prints various latencies in nanosecond unit.\n\n");
    ::printf(
        "{L[123],memory}_random_access: latency of loading&dereferening a "
        "random pointer\n"
        "   from L1: %ld KiB; L2: %ld KiB; L3: %ld MiB; \"memory\": %ld MiB.\n"
        "   (If glibc does not yield accurate non-zero values of cache sizes,\n"
        "   `numbers` skips measuring latencies of caches.)\n",
        L1_cache_size / KiB, L2_cache_size / KiB, L3_cache_size / MiB,
        multiples_of_L3 / MiB);
    ::printf(
        "branch_miss_penalty: latency due to a branch misprediction e.g. `if "
        "(x > 0)`.\n");
    ::printf("mutex_access: latency of `lock(); ++int; unlock();`.\n");
    ::printf("memory_copy_1MiB: latency for copying 1MiB across the RAM.\n");
    ::printf(
        "f{seek,read,write}_..._disk: latency of 1MiB-unit disk IO over a %ld "
        "MiB file.\n",
        multiples_of_L3 / MiB);
    ::printf("\n");
    ::printf("numbers [-h|--help] prints this help and quits.\n");
    ::printf(
        "numbers [-c|--concise] silences nanobench table of various "
        "metrics.\n");
    ::printf("\nFor more details, read ../notes.md or");
    ::printf(" https://github.com/jaeheum/numbers/blob/main/notes.md\n");
    return EXIT_SUCCESS;
  }

  ankerl::nanobench::Bench mutex_access;
  mutex_access.name(S(mutex_access)).output(outstream).run([&] {
    m.lock();
    ++mi;
    m.unlock();
  });

  void* dummy;
  std::vector<void*> memory;
  auto create_random_chain = [&](const size_t limit) {
    if (limit == 0) {
      return;
    }
    memory.resize(limit / sizeof(void*));
    for (size_t i = 0; i < memory.size() - 1; ++i) {
      memory[i] = reinterpret_cast<void*>(&memory[i + 1]);
    }
    memory[memory.size() - 1] = reinterpret_cast<void*>(&memory[0]);
    std::shuffle(memory.begin(), memory.end(),
                 ankerl::nanobench::Rng{std::random_device{}()});
  };

  auto chase_pointers = [&] {
    auto x = &memory[0];
    size_t count = memory.size();
    while (count--) {
      x = reinterpret_cast<void**>(*x);
    }
    dummy = *x;
  };

  create_random_chain(L1_cache_size);
  ankerl::nanobench::Bench L1_random_access;
  if (L1_cache_size != 0) {
    L1_random_access.name(S(L1_random_access))
        .output(outstream)
        .run(chase_pointers);
  }

  create_random_chain(L2_cache_size);
  ankerl::nanobench::Bench L2_random_access;
  if (L2_cache_size != 0) {
    L2_random_access.name(S(L2_random_access))
        .output(outstream)
        .run(chase_pointers);
  }

  create_random_chain(L3_cache_size);
  ankerl::nanobench::Bench L3_random_access;
  if (L3_cache_size != 0) {
    L3_random_access.name(S(L3_random_access))
        .output(outstream)
        .run(chase_pointers);
  }

  create_random_chain(multiples_of_L3);
  ankerl::nanobench::Bench memory_random_access;
  memory_random_access.name(S(memory_random_access))
      .epochs(1)
      .epochIterations(1)
      .output(outstream)
      .run(chase_pointers);

  // measure branch misprediction penalty from comparison of filtering
  // sorted array vs unsorted one. c.f.
  // https://stackoverflow.com/questions/11227809/why-is-it-faster-to-process-a-sorted-array-than-an-unsorted-array
  const auto c = std::max(L3_cache_size, multiples_of_L3 / sizeof(int)); 
  std::vector<int> vi(c);
  std::iota(std::begin(vi), std::end(vi), static_cast<int>(-c / 2));
  ankerl::nanobench::Bench sorted_memory_branch_mispredictions;
  int y;
  auto positive_only = [&] {
    for (const auto& x : vi) {
      if (x > 0) {
        y = x;
      }
    }
  };
  sorted_memory_branch_mispredictions
      .name(S(sorted_memory_branch_mispredictions))
      .output(outstream)
      .run(positive_only);

  ankerl::nanobench::Bench unsorted_memory_branch_mispredictions;
  std::shuffle(vi.begin(), vi.end(),
               ankerl::nanobench::Rng{std::random_device{}()});
  unsorted_memory_branch_mispredictions
      .name(S(unsorted_memory_branch_mispredictions))
      .output(outstream)
      .run(positive_only);

  const std::string_view chars(reinterpret_cast<const char*>(&memory[0]),
                               multiples_of_L3);
  ankerl::nanobench::Bench memory_copy_1MiB;
  memory_copy_1MiB.name(S(memory_copy_1MiB)).output(outstream).run([&] {
    std::string t;
    for (size_t i = 0; i < chars.size(); i += MiB) {
      t = chars.substr(i, MiB);
    }
  });

  ankerl::nanobench::Bench fwrite_1MiB_to_disk;
  ankerl::nanobench::Bench fseek_from_disk;
  ankerl::nanobench::Bench fread_1MiB_from_disk;
  std::error_code ec;
  const fs::space_info si = fs::space(fs::current_path(), ec);
  if (si.available > multiples_of_L3) {
    fwrite_1MiB_to_disk.name(S(fwrite_1MiB_to_disk))
        .epochs(1)
        .epochIterations(1)
        .output(outstream)
        .run([&] {
          FILE* fp = std::fopen(p.c_str(), "wb");
          assert(fp);
          std::fwrite(chars.data(), sizeof chars[0], chars.size(), fp);
          std::fclose(fp);
        });

    fseek_from_disk.name(S(fseek_from_disk))
        .epochs(1)
        .epochIterations(1)
        .output(outstream)
        .run([&] {
          FILE* fp = std::fopen(p.c_str(), "rb");
          assert(fp);
          for (size_t i = 0; i < fs::file_size(p); i += MiB) {
            std::fseek(fp, i, SEEK_SET);
          }
          std::fclose(fp);
        });

    fread_1MiB_from_disk.name(S(fread_1MiB_from_disk))
        .epochs(1)
        .epochIterations(1)
        .output(outstream)
        .run([&] {
          FILE* fp = std::fopen(p.c_str(), "rb");
          assert(fp);
          std::vector<char> cs(MiB);
          for (size_t i = 0; i < fs::file_size(p); i += MiB) {
            std::fread(&cs[0], sizeof cs[0], MiB, fp);
          }
          std::fclose(fp);
        });
  }

  auto v = [](const ankerl::nanobench::Result r, const std::string& s) {
    return r.fromString(s);
  };

  auto branchmisses = [&](const ankerl::nanobench::Bench& b) {
    auto branchmisses = 0.0;
    for (auto& x : b.results()) {
      branchmisses += x.median(v(x, "branchmisses"));
    }
    return branchmisses / b.results().size();
  };

  // XXX dedup
  auto cpucycles = [&](const ankerl::nanobench::Bench& b, const size_t denom) {
    auto cpucycles = 0.0;
    for (auto& x : b.results()) {
      cpucycles += x.median(v(x, "cpucycles"));
    }
    return cpucycles / b.results().size() / denom;
  };

  // XXX latency_per_element
  auto latency = [&](const ankerl::nanobench::Bench& b, const size_t denom) {
    auto latency = 0.0;
    for (auto& x : b.results()) {
      latency += x.median(v(x, "elapsed"));
    }
    return latency * std::chrono::nanoseconds(std::chrono::seconds(1)).count() /
           b.results().size() / denom;
  };

  auto sorted_branchmisses = branchmisses(sorted_memory_branch_mispredictions);
  auto unsorted_branchmisses =
      branchmisses(unsorted_memory_branch_mispredictions);
  auto sorted_latency = latency(sorted_memory_branch_mispredictions, 1UL);
  auto unsorted_latency = latency(unsorted_memory_branch_mispredictions, 1UL);
  auto penalty = (unsorted_latency - sorted_latency) /
                 (unsorted_branchmisses - sorted_branchmisses);
  auto sorted_latency_cycles =
      cpucycles(sorted_memory_branch_mispredictions, 1UL);
  auto unsorted_latency_cycles =
      cpucycles(unsorted_memory_branch_mispredictions, 1UL);
  auto penalty_cycles = (unsorted_latency_cycles - sorted_latency_cycles) /
                        (unsorted_branchmisses - sorted_branchmisses);

  if (!cmdline[{"-c", "--concise"}]) {
    ::printf("\n");
  }
  static const char* fmt_ns_cyc = "%-30s %10.1f ns %10.1f cycles\n";
  static const char* fmt_ns = "%-30s %10.1f ns\n";

  if (std::isnormal(penalty)) {
    if (L1_cache_size != 0) { 
      ::printf(fmt_ns_cyc, S(L1_random_access),
               latency(L1_random_access, L1_cache_size / sizeof(void*)),
               cpucycles(L1_random_access, L1_cache_size / sizeof(void*)));
    }
    if (L2_cache_size != 0) { 
      ::printf(fmt_ns_cyc, S(L2_random_access),
               latency(L2_random_access, L2_cache_size / sizeof(void*)),
               cpucycles(L2_random_access, L2_cache_size / sizeof(void*)));
    }
    if (L3_cache_size != 0) { 
      ::printf(fmt_ns_cyc, S(L3_random_access),
               latency(L3_random_access, L3_cache_size / sizeof(void*)),
               cpucycles(L3_random_access, L3_cache_size / sizeof(void*)));
    }
    ::printf(fmt_ns_cyc, S(memory_random_access),
             latency(memory_random_access, multiples_of_L3 / sizeof(void*)),
             cpucycles(memory_random_access, multiples_of_L3 / sizeof(void*)));
    ::printf(fmt_ns_cyc, S(branch_miss_penalty), penalty, penalty_cycles);
    ::printf(fmt_ns_cyc, S(mutex_access), latency(mutex_access, 1UL),
             cpucycles(mutex_access, 1UL));
    ::printf(fmt_ns_cyc, S(memory_copy_1MiB),
             latency(memory_copy_1MiB, chars.size() / MiB),
             cpucycles(memory_copy_1MiB, chars.size() / MiB));
    if (si.available > multiples_of_L3) {
      ::printf(fmt_ns_cyc, S(fseek_from_disk),
               latency(fseek_from_disk, fs::file_size(p) / MiB),
               cpucycles(fseek_from_disk, fs::file_size(p) / MiB));
      ::printf(fmt_ns_cyc, S(fread_1MiB_from_disk),
               latency(fread_1MiB_from_disk, fs::file_size(p) / MiB),
               cpucycles(fread_1MiB_from_disk, fs::file_size(p) / MiB));
      ::printf(fmt_ns_cyc, S(fwrite_1MiB_to_disk),
               latency(fwrite_1MiB_to_disk, fs::file_size(p) / MiB),
               cpucycles(fwrite_1MiB_to_disk, fs::file_size(p) / MiB));
    }
  } else {
    if (L1_cache_size != 0) { 
      ::printf(fmt_ns, S(L1_random_access),
               latency(L1_random_access, L1_cache_size / sizeof(void*)));
    }
    if (L2_cache_size != 0) { 
      ::printf(fmt_ns, S(L2_random_access),
               latency(L2_random_access, L2_cache_size / sizeof(void*)));
    }
    if (L3_cache_size != 0) { 
      ::printf(fmt_ns, S(L3_random_access),
               latency(L3_random_access, L3_cache_size / sizeof(void*)));
    }
    ::printf(fmt_ns, S(memory_random_access),
             latency(memory_random_access, multiples_of_L3 / sizeof(void*)));
    // skip printing branch_miss_penalty
    ::printf(fmt_ns, S(mutex_access), latency(mutex_access, 1UL));
    ::printf(fmt_ns, S(memory_copy_1MiB),
             latency(memory_copy_1MiB, chars.size() / MiB));
    if (si.available > multiples_of_L3) {
      ::printf(fmt_ns, S(fseek_from_disk),
              latency(fseek_from_disk, fs::file_size(p) / MiB));
      ::printf(fmt_ns, S(fread_1MiB_from_disk),
               latency(fread_1MiB_from_disk, fs::file_size(p) / MiB));
      ::printf(fmt_ns, S(fwrite_1MiB_to_disk),
               latency(fwrite_1MiB_to_disk, fs::file_size(p) / MiB));
    }
  }
  return EXIT_SUCCESS;
}
