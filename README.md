# Lock Free

## Objective

I'm was curious about lock free data structures and wanted to try and write one.

## The Q

This project implements a lock-free, fixed-size circular queue designed for single-producer, single-consumer (SPSC) use cases.

It does not support multiple threads pushing or popping at the same time.

## Build

From the root of the project, build with CMake:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run Tests

The project uses [Catch2](https://github.com/catchorg/Catch2) for testing.

You can run all tests or filter specific ones:

```
./build/tests
```

## Benchmarking

From what i read the most important metric for a lock free data struct is throughput. This was measured by blocks_of_data / seconds.

Also the 64 bit aligned read and right index help reduce cache misses.

Here's an example of performance statistics using perf:

```
$ perf stat -e instructions,cycles,cache-misses,cache-references ./build/P1C1
```
```
Total time: 101197 us
Throughput: 9.88172e+06 blocks/sec

 Performance counter stats for './build/P1C1':

        99,485,421      cpu_atom/instructions/u          #    0.19  insn per cycle              (21.27%)
       135,138,477      cpu_core/instructions/u          #    0.16  insn per cycle              (78.73%)
       516,844,579      cpu_atom/cycles/u                                                       (21.27%)
       837,896,679      cpu_core/cycles/u                                                       (78.73%)
            80,796      cpu_atom/cache-misses/u          #    0.86% of all cache refs           (21.27%)
             1,254      cpu_core/cache-misses/u          #    0.01% of all cache refs           (78.73%)
         9,419,045      cpu_atom/cache-references/u                                             (21.27%)
        16,050,360      cpu_core/cache-references/u                                             (78.73%)

       0.104833140 seconds time elapsed

       0.197240000 seconds user
       0.007974000 seconds sys

```
### Take Away
- 9.88 blocks_processed / second
- Low cache misses
