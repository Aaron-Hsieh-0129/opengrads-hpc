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
