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
// - `intptr_t*` instead of `void*`
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

  std::ostream* outstream = nullptr;
  argh::parser cmdline(argv);
  if (cmdline[{"-v", "--verbose"}]) {
    // print nanobench table output by setting bench.output(outstream)
    outstream = &std::cout;
  }
  if (cmdline[{"-h", "--help"}]) {
    // XXX reorg the code as a table of Bench/{code, param, docstring}?
    ::printf("numbers prints various latencies in nanosecond unit.\n\n");
    ::printf(
        "{L[123],memory}_random_access: latency of loading&dereferening a "
        "random pointer\n"
        "   from L1: %ld KiB; L2: %ld KiB; L3: %ld MiB; \"memory\": %ld MiB.\n",
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
        "numbers [-v|--verbose] prints nanobench table of various metrics as "
        "well.\n");
    return EXIT_SUCCESS;
  }

  ankerl::nanobench::Bench mutex_access;
  mutex_access.name(S(mutex_access)).output(outstream).run([&] {
    m.lock();
    ++mi;
    m.unlock();
  });

  intptr_t* dummy;
  std::vector<intptr_t*> memory;
  auto create_random_chain = [&](const size_t limit) {
    memory.resize(limit / sizeof(intptr_t*));
    for (size_t i = 0; i < memory.size() - 1; ++i) {
      memory[i] = reinterpret_cast<intptr_t*>(&memory[i + 1]);
    }
    memory[memory.size() - 1] = reinterpret_cast<intptr_t*>(&memory[0]);
    std::shuffle(memory.begin(), memory.end(),
                 ankerl::nanobench::Rng{std::random_device{}()});
  };

  auto chase_pointers = [&] {
    auto x = &memory[0];
    size_t count = memory.size();
    while (count--) {
      x = reinterpret_cast<intptr_t**>(*x);
    }
    dummy = *x;
  };

  create_random_chain(L1_cache_size);
  ankerl::nanobench::Bench L1_random_access;
  L1_random_access.name(S(L1_random_access))
      .output(outstream)
      .run(chase_pointers);

  create_random_chain(L2_cache_size);
  ankerl::nanobench::Bench L2_random_access;
  L2_random_access.name(S(L2_random_access))
      .output(outstream)
      .run(chase_pointers);

  create_random_chain(L3_cache_size);
  ankerl::nanobench::Bench L3_random_access;
  L3_random_access.name(S(L3_random_access))
      .output(outstream)
      .run(chase_pointers);

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
  std::vector<int> vi(L3_cache_size);
  std::iota(std::begin(vi), std::end(vi), static_cast<int>(-L3_cache_size / 2));
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

  ankerl::nanobench::Bench fseek_from_disk;
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

  ankerl::nanobench::Bench fread_1MiB_from_disk;
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

  static const char fmt[] = "%s %.1f\n";
  ::printf(fmt, S(L1_random_access),
           latency(L1_random_access, L1_cache_size / sizeof(intptr_t*)));
  ::printf(fmt, S(L2_random_access),
           latency(L2_random_access, L2_cache_size / sizeof(intptr_t*)));
  ::printf(fmt, S(L3_random_access),
           latency(L3_random_access, L3_cache_size / sizeof(intptr_t*)));
  ::printf(fmt, S(memory_random_access),
           latency(memory_random_access, multiples_of_L3 / sizeof(intptr_t*)));
  ::printf(fmt, S(branch_miss_penalty), penalty);
  ::printf(fmt, S(mutex_access), latency(mutex_access, 1UL));
  ::printf(fmt, S(fseek_from_disk),
           latency(fseek_from_disk, fs::file_size(p) / MiB));
  ::printf(fmt, S(memory_copy_1MiB),
           latency(memory_copy_1MiB, chars.size() / MiB));
  ::printf(fmt, S(fread_1MiB_from_disk),
           latency(fread_1MiB_from_disk, fs::file_size(p) / MiB));
  ::printf(fmt, S(fwrite_1MiB_to_disk),
           latency(fwrite_1MiB_to_disk, fs::file_size(p) / MiB));
  return EXIT_SUCCESS;
}
