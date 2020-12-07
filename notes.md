# notes on numbers

## calibration

If you do not set CPU governor and `pyperf system tune`, `nanobench` will warn: 
```                                                                             
Warning, results might be unstable:                                             
* CPU frequency scaling enabled: CPU 0 between 1,200.0 and 3,800.0 MHz          
* CPU governor is 'schedutil' but should be 'performance'                       
* Turbo is enabled, CPU frequency will fluctuate                                

Recommendations                                                                 
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyper
f
```

`print-numbers` script takes care of running `pyperf`,
setting `scaling_governor`, and others.

## numbers internals

- L1, L2, L3 sizes come from glibc.
  - not tested with musl.
  - so far only run on linux, not tested with other OSes.
  - maybe this is portable across different hardware on linux, but not tested.
- For the “main memory”, set its size to *L3 size × 16*.
  - this value should sufficiently large enough to be realistic, but small enough for quick-and-dirty measurement.

### mutex measurement

- Mutex measurement is simple: measure the time for mutex.lock(); ++int; mutex.unlock().

**n.b.** this is done in single-thread.

### memory latency measurement

- Fill L1, L2, L3, main memory with a linked list of pointers pointing to the next item in the list; shuffle the list.
- Then measure the time to load a pointer, and dereference it to go to the next
in the list; repeat over the list.
  - This technique is common across many tools listed in [README.md](README.md).

**n.b.** see [README.md](README.md) latency measurements discussions and tools
for more precise ways of measuring memory latencies.

### branch misprediction penalty measurement

- Branch misprediction penalty:
  - Create an array of *-N .. N-1*.
  - Apply a filter `if(x>0)y=x;` over the array.
    - Sorted array would take t0 time.
    - Shuffle the array.
    - Apply the filter again and observe this takes t1 time and t1>t0.
  - Nanobench supports hardware performance counters including branch mispredictions: *penalty = (t1-t0)/mispredictions*.

**n.b.** branch misprediction penalty calculation fails when the kernel
does not let users read hardware performance counter.
In that case, `numbers` omits printing the penalty value.

In the table of benchmarks, many columns like `IPC` and `miss%` may be missing. 
One can still observe branch mispredition effect by noticing the difference
n `ns/op` and `total` values between `sorted_memory_branch_mispredictions` and
`unsorted_memory_branch_mispredictions` benchmarks. 

See [this](https://www.kernel.org/doc/html/latest/admin-guide/perf-security.html)about accessing hardware performance counters.

### f{write,read,seek} latency measurement

- `fwrite()` the memory data to a file in `$PWD`, do `fseek()` and `fread()` the file in 1MiB unit.
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

