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
$ numbers -h
numbers prints various latencies in nanosecond unit.

{L[123],memory}_random_access: latency of loading&dereferening a random pointer
   from L1: 32 KiB; L2: 256 KiB; L3: 15 MiB; "memory": 256 MiB.
branch_miss_penalty: latency due to a branch misprediction e.g. `if (x > 0)`.
mutex_access: latency of `lock(); ++int; unlock();`.
memory_copy_1MiB: latency for copying 1MiB across the RAM.
f{seek,read,write}_…_disk: latency of 1MiB-unit disk IO over a 256 MiB file.

numbers [-h|--help] prints this help and quits.
numbers [-v|--verbose] prints nanobench table of various metrics as well.
```

```
$ /usr/bin/time numbers
L1_random_access 1.3
L2_random_access 5.6
L3_random_access 26.6
memory_random_access 85.8
branch_miss_penalty 6.3
mutex_access 16.5
fseek_from_disk 1510.7
memory_copy_1MiB 105468.9
fread_1MiB_from_disk 134367.1
fwrite_1MiB_to_disk 609395.0
6.00user 0.35system 0:06.43elapsed 98%CPU (0avgtext+0avgdata 328196maxresident)k
0inputs+524288outputs (0major+85221minor)pagefaults 0swaps
```

The last few lines is the output of `/usr/bin/time` showing
time and memory usage ("memory" + sizeof(int) × L3).

## prerequisites

- recent linux/glibc on x86-64 with a 1 or 2 GB of free RAM and disk space
- gcc 9 or 10 for c++17 support; clang 10 or later ok
- recent glibc for `sysconf()` and `_SC_LEVEL[123]_*CACHE_SIZE` symbols
- GNU Make
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
