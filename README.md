# numbers

**2020-11-28**

`numbers` is a *quick-and-dirty* code that prints some of the
["latency numbers every programmer should know" 1](https://gist.github.com/hellerbarde/2843375),
[2](http://norvig.com/21-days.html#answers), [3](https://gist.github.com/jboner/2841832), [4](https://colin-scott.github.io/personal_website/research/interactive_latency.html).

**n.b.** it's for linux on x86-64 with c++17 compiler only (gcc 9, 10; clang 10, 11). 

```
$ git clone https://github.com/jaeheum/numbers.git
$ cd numbers
# a quick-and-dirty run:
$ make all && ./build/numbers
g++ -Wall -Wextra -Wpedantic -Ofast --std=c++17  -Iinclude -c src/nanobench.cc -o build/nanobench.o
g++ -Wall -Wextra -Wpedantic -Ofast --std=c++17  -Iinclude -c src/numbers.cc -o build/numbers.o
g++  -o build/numbers build/*.o -lpthread
Warning, results might be unstable:
* CPU governor is 'schedutil' but should be 'performance'

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf

|               ns/op |                op/s |    err% |          ins/op |          cyc/op |    IPC |         bra/op |   miss% |     total | benchmark
|--------------------:|--------------------:|--------:|----------------:|----------------:|-------:|---------------:|--------:|----------:|:----------
|               16.50 |       60,604,416.47 |    0.0% |           63.01 |           56.08 |  1.124 |          18.00 |    0.0% |      0.00 | `mutex_access`
|            5,217.33 |          191,668.80 |    0.6% |       12,301.00 |       17,736.67 |  0.694 |       4,097.00 |    0.0% |      0.00 | `L1_random_access`
|          250,328.00 |            3,994.76 |    0.2% |       98,329.00 |      850,748.00 |  0.116 |      32,769.00 |    0.0% |      0.00 | `L2_random_access`
|       52,028,319.00 |               19.22 |    1.5% |    5,898,281.00 |  176,516,236.00 |  0.033 |   1,966,097.00 |    0.0% |      0.60 | `L3_random_access`
|    2,796,066,612.00 |                0.36 |    0.0% |  100,664,178.00 |9,487,319,166.00 |  0.011 |  33,555,290.00 |    0.0% |      2.80 | `memory_random_access`
|        8,813,073.00 |              113.47 |    1.7% |  110,100,499.00 |   29,845,812.00 |  3.689 |  31,457,284.00 |    0.0% |      0.10 | `sorted_memory_branch_mispredictions`
|       57,787,781.00 |               17.30 |    0.1% |  110,100,513.00 |  196,153,140.00 |  0.561 |  31,457,298.00 |   25.0% |      0.64 | `unsorted_memory_branch_mispredictions`
|       28,025,098.00 |               35.68 |    0.1% |       23,985.00 |   94,954,316.00 |  0.000 |       4,964.00 |    0.3% |      0.31 | `memory_copy_1MiB`
|      156,300,394.00 |                6.40 |    0.0% |        4,350.00 |       53,176.00 |  0.082 |         796.00 |   21.6% |      0.16 | `fwrite_1MiB_to_disk`
|          388,215.00 |            2,575.89 |    0.0% |       81,101.00 |       77,384.00 |  1.048 |      18,564.00 |    0.6% |      0.00 | `fseek_from_disk`
|       34,513,692.00 |               28.97 |    0.0% |       67,884.00 |      396,406.00 |  0.171 |      15,174.00 |    2.5% |      0.03 | `fread_1MiB_from_disk`

L1_random_access                      1.3 ns        4.3 cycles
L2_random_access                      7.6 ns       26.0 cycles
L3_random_access                     26.5 ns       89.8 cycles
memory_random_access                 83.3 ns      282.7 cycles
branch_miss_penalty                   6.2 ns       21.2 cycles
mutex_access                         16.5 ns       56.1 cycles
fseek_from_disk                    1516.5 ns      302.3 cycles
memory_copy_1MiB                 109473.0 ns   370915.3 cycles
fread_1MiB_from_disk             134819.1 ns     1548.5 cycles
fwrite_1MiB_to_disk              610548.4 ns      207.7 cycles
```

### quick and dirty

`numbers`'s output comes from simplistic, best-effort, low-cost, fast measurements.

**n.b.** Read [notes.md](notes.md) for making `numbers` less quick-and-dirty,
along with information about internals of `numbers`, and more advanced tools.

There are also [known issues](https://github.com/jaeheum/numbers/issues).

### no cycles column in the output

Depending on Linux configuration, `numbers` may not be able to access
hardware performance counters. In this case, `numbers` does not print
`branch_miss_penalty` row and `cycles` column in the output.

## License

MIT License

## Acknowledgement

`numbers` benefits from
- code derived from https://github.com/afborchert/pointer-chasing (MIT)
- [nanobench](https://nanobench.ankerl.com/reference.html) for microbenchmarking suite (MIT)
- [argh](https://github.com/adishavit/argh) for command line handling (BSD-3)
- [a stackflow post on sorted vs unsorted array](https://stackoverflow.com/questions/11227809/why-is-it-faster-to-process-a-sorted-array-than-an-unsorted-array)

