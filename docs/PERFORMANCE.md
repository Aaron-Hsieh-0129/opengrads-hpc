# Calculation performance

opengrads-hpc can use OpenMP for common in-memory grid calculations. The default
is **4 calculation threads**. OpenMP is detected automatically by
`configure`; no extra user action is needed with a supported GCC, Clang, or
vendor compiler.

## Choosing the CPU count

Choose the setting that fits the machine and workload:

```text
ga-> q threads
Calculation threads = 4 (OpenMP enabled)
ga-> set threads 8
Calculation threads set to 8
```

The same choice can be made at startup:

```bash
./opengrads -j 8
GA_NUM_THREADS=8 ./opengrads
```

Precedence is `-j N`, then `GA_NUM_THREADS`, then the default of 4. A script
may change the count at any time with `set threads N`. Use `set threads 1` for
a serial calculation or for output-comparison testing.

Do not automatically select every logical CPU on a shared node. Start with 4,
then benchmark 2, 4, 8, and so on against the real command script. Memory
bandwidth often limits grid arithmetic before all cores are useful.

## Accelerated calculations

The implementation parallelizes independent cells in the hot, thread-safe
parts of expression evaluation, including:

- grid/scalar arithmetic, comparisons, powers, masks, `mag`, and `atan2`;
- `sqrt`, trigonometric functions, `abs`, `exp`, `log`, and `log10`;
- `mean`, `ave`, `sum`, `sumg`, `min`, `max`, `minloc`, and `maxloc`;
- `amean`, `aave`, `asum`, and `atot` area reductions;
- `gint`, `vint`, `const`, `cdiff`, and `smth9`.

Small grids remain serial because creating a thread team costs more than it
saves. The current crossover is 32,768 cells. File reads, expression parsing,
coordinate conversion setup, and graphics remain serial because the legacy
core keeps mutable global state in those paths.

## Time averages of BP5 data

`ave`, `mean`, `sum`, `sumg`, `min`, `max`, `minloc`, and `maxloc` over time
evaluate their expression once per time step, and each step used to be a
separate read. When the expression is a plain variable of a BP5 dataset that
is the default file (`ave(th,t=1,t=1441)`, not `ave(th*2,...)` or a defined
variable), the steps are instead read many at a time: up to 256 steps, or
64 MB, go to ADIOS2 as one batch, which its reader threads serve in parallel,
and they are folded into the result on the calculation threads. Each grid
point still takes its steps in order through the same accumulation code, so
the result matches the step-by-step path; the BP5 regression compares the two
for exact equality at one and at four threads. Any other expression, file
format, or a step outside the file goes step by step as before.

BP5 reads of a vertical section (x-z or y-z) and of a profile are also one
read per step now, rather than one per level.

On a 96 × 96 × 300 dataset of 241 steps, written as 16 blocks per step the way
a 16-rank run writes it, with the data in the page cache and 4 threads:

| Request | Before | After |
| --- | --- | --- |
| x-z section, `ave(th,t=1,t=241)` | 5.5 s | 0.06 s |
| profile, `ave(th,t=1,t=241)` | 0.19 s | 0.01 s |
| x-y map, `ave(th,t=1,t=241)` | 0.04 s | 0.02 s |
| `define m = ave(th,t=1,t=241)` over all 300 levels | 13.6 s | 3.3 s |

The 3-D `define` evaluates the average one level at a time, so it remains
bounded by reading 300 separate planes per batch of steps; with one thread it
takes 4.0 s.

## Reproducibility of parallel reductions

Parallel reductions can differ from a one-thread result in the last few
floating-point bits because additions may be grouped differently. Missing
value masks and scientific semantics are unchanged. Use an appropriate
tolerance rather than requiring bit-for-bit equality for nontrivial sums.

## Build controls and verification

Force a serial build when required:

```bash
./configure --disable-openmp
```

To require OpenMP and fail configuration when it is unavailable:

```bash
./configure --enable-openmp
```

The configuration banner and `q config` report whether OpenMP was compiled
in. After building `grads` and `libgxdummy.la`, run:

```bash
OPENGRADS_BUILD_ROOT=/path/to/build ./pytests/TestOpenMP.sh
```

The test exercises a 256x256 grid, checks the four-thread default and runtime
controls, and compares common calculation output at one and four threads.

## Reproducible large-case benchmark

Run the supplied benchmark after building `grads` and `libgxdummy.la`:

```bash
OPENGRADS_BUILD_ROOT="$PWD/build" ./benchmarks/BenchmarkOpenMP.sh
```

It creates a sparse 2048 x 2048 float dataset with six time steps, warms the
file cache, and runs six repetitions of a mixed workload: elementwise
transcendental arithmetic, `amean`, `aave`, `asum`, `mean`, and `sum`. It then
reports the median of three complete runs at one CPU and at the selected CPU
count. Override its defaults with `OPENGRADS_BENCH_THREADS`,
`OPENGRADS_BENCH_SIZE`, `OPENGRADS_BENCH_TIMES`,
`OPENGRADS_BENCH_REPEATS`, and `OPENGRADS_BENCH_TRIALS`.

On 22 August 2026, the default case produced this result on an aarch64 system
where `nproc` exposed five CPUs (heterogeneous Cortex-X925/Cortex-A725 host):

| Calculation CPUs | Median seconds | Speedup |
|---:|---:|---:|
| 1 (original/single CPU) | 9.31 | 1.00x |
| 4 | 5.05 | 1.84x |

This is an end-to-end result for one synthetic workload, not a universal
scaling promise. File access, parsing, allocation, and other legacy global
state remain serial, so pure arithmetic kernels can scale better while
I/O-heavy scripts can scale less.
