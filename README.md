# Escort

## How to build

### Required
This repository depends on `libhwloc` and `libpthread`.

### Step 1. jemalloc

Escort uses a customized jemalloc. Fill in the /jemalloc submodule
and build required libraries by the following command:

```
tools/build-jemalloc.sh
```

This command generates

  * jemalloc/lib/libjemallocescort.so and
  * jemalloc/lib/libjemallocescortdbg.so (with debugging facility enabled).

Alternatively the following command compiles jemalloc.

```
$ cd jemalloc
$ ./autogen.sh
$ ./configure --with-jemalloc-prefix=_escort_internal_ --disable-cxx --with-install-suffix=escort
$ make
```

`make install` is not required for benchmarking.  Benchmark script sets
library paths in environment variables.

### Step 2. escort

Build `bin/libescort.so` and `bin/libescortdbg.so`.

```
$ make
```

If jemalloc is in a different location, modify `config.h` so that it can
include jemallocescort.h.

## Test

In `test` directory, execute the following command.

```
./run.sh <NVM file prefix>
```

<NVM file prefix> is a prefix of the paths of the files for NVM heap
and metadata. They should be in NVM in real use. For the test,
they can be in a normal disk.

Remark that this test script creates files up to 50 GB in total.

## Benchmark

Benchmark programs require additional libraries: `libevent` and `libjemalloc`.
Note that this `libjemalloc` is a standard one, not `libjemallocescort.so`.


TODO

## Compile options

  * ESCORT_PROF: Enalbe profiling feature that may affect preformance.
  * MEASURE: Enable measurement for ops/sec.

## API
Escort shows APIs below(you can check the attributes of each API at `src/Escort.hpp`)
- `Escort_init`
  - create NVM images ,and if with recovery mode this function recover DRAM heap and the memory allocator metadata
- `Escort_is_recovery`
  - check whether NVM images are dirty, if NVM images are dirty, this function recognizes NVM images should be recovered
- `Esocrt_finalize`
- `Escort_thread_init`
- `Escort_thread_finalize`
- `Escort_begin_op`
- `Escort_end_op`
- `Escort_set_root`
- `Escort_get_root`
- `Escort_remove_root`
- `PNEW`, `PDELETE`
  - array `new/delete` operations are not supported
- `PWRITE`
  - array `write` operations are not supported.
- `Escort_write_region`
  - if array `write` operations, use this macro before `write` operations
- `Escort_malloc`
- `Escort_free`
