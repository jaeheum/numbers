# numbers

**2020-11-28**

`numbers` is a *quick-and-dirty* code that prints some of the
["latency numbers every programmer should know" 1](https://gist.github.com/hellerbarde/2843375),
[2](http://norvig.com/21-days.html#answers), [3](https://gist.github.com/jboner/2841832), [4](https://colin-scott.github.io/personal_website/research/interactive_latency.html).

## “Fast, Cheap, or Good? Pick two.”

`numbers` is fast & cheap, but it doesn't pretend to be good.
It is merely adequate.

```
$ git clone https://github.com/jaeheum/numbers.git
$ cd numbers
# a quick-and-dirty run:
$ make all && ./build/numbers
# or a bit less quick-and-dirty run:
$ make all && ./print-numbers

...

|               ns/op |                op/s |    err% |          ins/op |          cyc/op |    IPC |         bra/op |   miss% |     total | benchmark
|--------------------:|--------------------:|--------:|----------------:|----------------:|-------:|---------------:|--------:|----------:|:----------
|               16.50 |       60,600,848.53 |    0.0% |           63.01 |           56.10 |  1.123 |          18.00 |    0.0% |      0.00 | `mutex_access`
|            4,835.25 |          206,814.54 |    0.1% |       12,299.50 |       16,439.00 |  0.748 |       4,097.00 |    0.0% |      0.00 | `L1_random_access`
|          180,318.00 |            5,545.76 |    1.4% |       98,341.00 |      612,952.00 |  0.160 |      32,771.00 |    0.0% |      0.00 | `L2_random_access`
|       52,336,797.00 |               19.11 |    3.2% |    5,898,281.00 |  177,635,618.00 |  0.033 |   1,966,097.00 |    0.0% |      0.58 | `L3_random_access`
|    2,875,710,541.00 |                0.35 |    0.0% |  100,664,207.00 |9,758,240,346.00 |  0.010 |  33,555,319.00 |    0.0% |      2.88 | `memory_random_access`
|        9,299,396.00 |              107.53 |    0.7% |  110,100,499.00 |   31,486,754.00 |  3.497 |  31,457,284.00 |    0.0% |      0.10 | `sorted_memory_branch_mispredictions`
|       58,986,666.00 |               16.95 |    0.1% |  110,100,514.00 |  200,140,626.00 |  0.550 |  31,457,299.00 |   25.0% |      0.65 | `unsorted_memory_branch_mispredictions`
|       28,327,114.00 |               35.30 |    0.1% |       23,989.00 |   95,874,050.00 |  0.000 |       4,965.00 |    0.5% |      0.31 | `memory_copy_1MiB`
|      157,010,839.00 |                6.37 |    0.0% |        4,350.00 |       51,952.00 |  0.084 |         796.00 |   18.7% |      0.16 | `fwrite_1MiB_to_disk`
|          387,493.00 |            2,580.69 |    0.0% |       81,101.00 |       74,256.00 |  1.092 |      18,564.00 |    0.6% |      0.00 | `fseek_from_disk`
|       36,226,395.00 |               27.60 |    0.0% |       67,884.00 |      406,640.00 |  0.167 |      15,174.00 |    2.3% |      0.04 | `fread_1MiB_from_disk`

L1_random_access                      1.2
L2_random_access                      5.5
L3_random_access                     26.6
memory_random_access                 85.7
branch_miss_penalty                   6.3
mutex_access                         16.5
fseek_from_disk                    1513.6
memory_copy_1MiB                 110652.8
fread_1MiB_from_disk             141509.4
fwrite_1MiB_to_disk              613323.6
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

#### installation
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

See [notes](notes.md) and [issues](https://github.com/jaeheum/numbers/issues).
  
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
