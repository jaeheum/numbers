# notes on numbers

## calibration

If you do not set CPU governor and `pyperf system tune`, `nanobench` will warn: 
```                                                                             
Warning, results might be unstable:                                             
* CPU frequency scaling enabled: CPU 0 between 1,200.0 and 3,800.0 MHz          
* CPU governor is 'schedutil' but should be 'performance'                       
* Turbo is enabled, CPU frequency will fluctuate                                

Recommendations                                                                 
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf
```

`print-numbers` script takes care of running `pyperf`,
setting `scaling_governor`, and others. Be warned that `print-numbers`
expects many prerequisite binaries installed. Especially `pyperf` should
be installed as root (`sudo pip install pyperf`) so that
`sudo pyperf system tune` can run.

## numbers internals

L1, L2, L3 sizes come from glibc
- so far only tested on linux/x86-64, not tested with other OS/libc/hardware.
- maybe this is portable across different hardware on linux, but not tested.

For the “main memory”, set its size to *L3 size × 16*.
- this value should sufficiently large enough to be usable, but small enough for quick-and-dirty measurements.

### mutex measurement

Mutex measurement is simple: measure the time for
`mutex.lock(); ++int; mutex.unlock()`.

**n.b.** this is done in single-thread.

### memory latency measurement

Fill L1, L2, L3, main memory with a linked list of pointers pointing to the next item in the list; shuffle the list.
Measure the time to load a pointer, and dereference it to go to the next
in the list; repeat over the list.

**n.b.** This technique is common in **latency measurements discussions and
tools** listed below.

### branch misprediction penalty measurement

Branch misprediction penalty:
- Create an array of *-N .. N-1*.
- Apply a filter `if(x>0)y=x;` over the array.
  - Sorted array would take t0 time.
  - Shuffle the array.
  - Apply the filter again and observe this takes t1 time and t1>t0.

Nanobench supports hardware performance counters including branch mispredictions: *penalty = (t1-t0)/mispredictions*.

**n.b.** branch misprediction penalty calculation fails if the kernel
does not let users read hardware performance counter.
In that case, `numbers` omits printing the penalty value.

In the table of benchmarks, many columns like `IPC` and `miss%` may be missing. 
One can still observe branch mispredition effect by noticing the difference
n `ns/op` and `total` values between `sorted_memory_branch_mispredictions` and
`unsorted_memory_branch_mispredictions` benchmarks. 

See this [nanobench issue 36](https://github.com/martinus/nanobench/issues/36) for details.

### f{write,read,seek} latency measurement

`fwrite()` the “main memory” data to a file in `$PWD`, do `fseek()` and `fread()` the file in 1MiB unit.
- this is run once as a *low-effort* workaround for file caching effects.
- deletes the file at exit.

**n.b.** noticeable differences across file systems e.g. tmpfs vs disk.

## dependencies

`numbers` tries to use as few dependencies as possible:

- [pyperf](https://pyperf.readthedocs.io/en/latest/) to prepare the machine for 
measurement.                                                                    
  - it needs to be installed as root or system python package e.g. `sudo pip install pyperf`.
  - some linux distros lets you install it with their package managers
- [nanobench](https://nanobench.ankerl.com/index.html) already in `include/nanobench.h` (MIT license)
- [argh](https://github.com/adishavit/argh) already in `include/argh.h` (BSD-3 license)

Headers-only libraries like `nanobench` and `argh` are preferable
thanks to divergence in incompatible C++ package managers.

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

