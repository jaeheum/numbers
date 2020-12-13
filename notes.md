# notes on numbers

## warning and calibration

As noted in [README.md](README.md), `numbers` may print some warning like:
```                                                                             
Warning, results might be unstable:                                             
* CPU frequency scaling enabled: CPU 0 between 1,200.0 and 3,800.0 MHz          
* CPU governor is 'schedutil' but should be 'performance'                       
* Turbo is enabled, CPU frequency will fluctuate                                

Recommendations                                                                 
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf
```

Meaning of this warning is: typical default Linux
and hardware configuations allow CPU to change frequency depending on
given situations and that can result in inaccurate measurement.
(`numbers` uses `nanobench` performance test suite and it is actually
`nanobench` that issues this warning.)

Recommended courses of action:
- reduce noise of the system: e.g. stop browser and other non-essential processes.
- install `pyperf` as a root: `sudo pip install pyperf`
  - this is to enable running `sudo pyperf system tune`
  - some linux distributions may have `pyperf` as a system package
- run `print-numbers` script that wraps `pyperf` and `build/numbers`.
  - this results in better calibrated output

### benchmarking tips

Consult these for more advanced benchmark calibration tips e.g. isolation of cpu.
- https://pyperf.readthedocs.io/en/latest/system.html
- https://llvm.org/docs/Benchmarking.html
- https://nanobench.ankerl.com/reference.html

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

Fill L1, L2, L3, “main memory” with a linked list of pointers pointing to the
next one in the list; shuffle the list.
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

