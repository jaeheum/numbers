# numbers

**2020-11-28**

`numbers` is a *quick-and-dirty* code that prints some of the
["latency numbers every programmer should know" 1](https://gist.github.com/hellerbarde/2843375),
[2](http://norvig.com/21-days.html#answers), [3](https://gist.github.com/jboner/2841832), [4](https://colin-scott.github.io/personal_website/research/interactive_latency.html).

## “Fast, Cheap, or Good? Pick two.”

`numbers` is fast & cheap, but it doesn't pretend to be good.
It is merely adequate.

## sample output

```
$ make all
$ ./build/numbers

|               ns/op |                op/s |    err% |          ins/op |          cyc/op |    IPC |         bra/op |   miss% |     total | benchmark
|--------------------:|--------------------:|--------:|----------------:|----------------:|-------:|---------------:|--------:|----------:|:----------
|               16.50 |       60,606,060.61 |    0.0% |           63.01 |           56.10 |  1.123 |          18.00 |    0.0% |      0.00 | `mutex_access`
|            5,030.00 |          198,807.16 |    0.4% |       12,301.00 |       17,090.67 |  0.720 |       4,097.00 |    0.0% |      0.00 | `L1_random_access`
|          169,462.00 |            5,901.03 |    0.7% |       98,329.00 |      575,994.00 |  0.171 |      32,769.00 |    0.0% |      0.00 | `L2_random_access`
|       48,964,651.00 |               20.42 |    2.0% |    5,898,280.00 |  166,196,148.00 |  0.035 |   1,966,096.00 |    0.0% |      0.57 | `L3_random_access`
|    2,874,078,414.00 |                0.35 |    0.0% |  100,664,198.00 |9,754,137,498.00 |  0.010 |  33,555,310.00 |    0.0% |      2.87 | `memory_random_access`
|        8,781,445.00 |              113.88 |    0.8% |  110,100,499.00 |   29,623,078.00 |  3.717 |  31,457,284.00 |    0.0% |      0.10 | `sorted_memory_branch_mispredictions`
|       59,044,773.00 |               16.94 |    0.0% |  110,100,514.00 |  200,397,836.00 |  0.549 |  31,457,299.00 |   25.0% |      0.65 | `unsorted_memory_branch_mispredictions`
|       28,137,126.00 |               35.54 |    0.1% |       23,983.00 |   95,381,152.00 |  0.000 |       4,963.00 |    0.2% |      0.31 | `memory_copy_1MiB`
|      156,369,464.00 |                6.40 |    0.0% |        4,350.00 |       53,074.00 |  0.082 |         796.00 |   18.0% |      0.16 | `fwrite_1MiB_to_disk`
|          390,423.00 |            2,561.32 |    0.0% |       81,101.00 |       78,200.00 |  1.037 |      18,564.00 |    0.5% |      0.00 | `fseek_from_disk`
|       35,041,168.00 |               28.54 |    0.0% |       67,884.00 |      413,712.00 |  0.164 |      15,174.00 |    2.4% |      0.04 | `fread_1MiB_from_disk`
L1_random_access 1.2
L2_random_access 5.2
L3_random_access 24.9
memory_random_access 85.7
branch_miss_penalty 6.4
mutex_access 16.5
fseek_from_disk 1525.1
memory_copy_1MiB 109910.6
fread_1MiB_from_disk 136879.6
fwrite_1MiB_to_disk 610818.2
```
Help message:
```
$ ./build/numbers -h
numbers prints various latencies in nanosecond unit.

{L[123],memory}_random_access: latency of loading&dereferening a random pointer
   from L1: 32 KiB; L2: 256 KiB; L3: 15 MiB; "memory": 256 MiB.
branch_miss_penalty: latency due to a branch misprediction e.g. `if (x > 0)`.
mutex_access: latency of `lock(); ++int; unlock();`.
memory_copy_1MiB: latency for copying 1MiB across the RAM.
f{seek,read,write}_..._disk: latency of 1MiB-unit disk IO over a 256 MiB file.

numbers [-h|--help] prints this help and quits.
numbers [-c|--concise] silences nanobench table of various metrics.
```

## prerequisites

- recent linux/glibc on x86-64 with a 1 or 2 GB of free RAM and disk space
- gcc 9 or 10 for c++17 support; clang 10 or later ok
- GNU Make
- recent glibc for `sysconf()` and `_SC_LEVEL[123]_*CACHE_SIZE` symbols
- pthreads for mutex implementation
- [pyperf](https://pyperf.readthedocs.io/en/latest/) to prepare the machine for measurement.
- [nanobench](https://nanobench.ankerl.com/index.html) already in `include/nanobench.h` (MIT license)
- [argh](https://github.com/adishavit/argh) already in `include/argh.h` (BSD-3 license)

If you do not set CPU governor and `pyperf system tune`, `nanobench` will warn:
```
Warning, results might be unstable:
* CPU frequency scaling enabled: CPU 0 between 1,200.0 and 3,800.0 MHz
* CPU governor is 'schedutil' but should be 'performance'
* Turbo is enabled, CPU frequency will fluctuate

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf
```

## build and run

```
git clone https://github.com/jaeheum/numbers.git
cd numbers
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
sudo pyperf system tune # follow its direction
# set core pinning if desired
make # puts numbers binary in $(pwd)/build
./build/numbers
make clean
```

`numbers -h` prints help message; `-v` prints more metrics from nanobench.

To install `numbers` to `$HOME/.local/bin/numbers`:

```
make install # builds and installs numbers
make uninstall # rm -f $HOME/.local/bin/numbers
make clean # rm -f build/*
```

Run `sudo make PREFIX=/usr/local install` to install `numbers` system-wide.


## quick and dirty

Beware [observational errors](https://en.wikipedia.org/wiki/Observational_error):

- output will vary across runs by low double digit percentage
- IO measurements are run once as a *low-effort* to ignore file-caching effect
- users need to manually handle many sources of systematic errors:
  - CPU frequency scaling
  - IO scheduling, file caching, etc
  - workload isolation, core pinning, etc.

`numbers`'s output is simplistic, best-effort, low-cost, fast measurements.
For precision measurement see other tools listed below.

See [issues](https://github.com/jaeheum/numbers/issues).
  
## License

MIT License

## further readings

### numbers

- https://gist.github.com/hellerbarde/2843375
- http://norvig.com/21-days.html#answers
- https://gist.github.com/jboner/2841832
- https://colin-scott.github.io/personal_website/research/interactive_latency.html
- https://github.com/sirupsen/napkin-math
- http://ithare.com/infographics-operation-costs-in-cpu-clock-cycles/

### latency measurements discussions and tools

- http://www.bitmover.com/lmbench/
- https://www.forrestthewoods.com/blog/memory-bandwidth-napkin-math/
- https://github.com/afborchert/pointer-chasing
- https://github.com/ssvb/tinymembench
- https://software.intel.com/content/www/us/en/develop/articles/intelr-memory-latency-checker.html
- https://github.com/Kobzol/hardware-effects

### benchmarking tips

- https://nanobench.ankerl.com/reference.html
- https://llvm.org/docs/Benchmarking.html
