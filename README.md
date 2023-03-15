# Escort
## How to build
### Required
This repository depends on `libhwloc`, `libjemalloc`, and `libpthread`.

### command
~~~
$ make
~~~

Makefile creates `bin/libescort.so`. 
If `bin/`, `obj/` direcotries do not exist, 
these direcotries will be created.
And if `jemalloc` hasn't been built yet, jemalloc libraries will be created.

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
