// This is the only file the user can see.
#ifndef ESCORT_HPP
#define ESCORT_HPP

#include <cstddef>
#include <cstdint>
#include <iostream>

void escort_init(const char* nvm_path, std::uint32_t checkpointing_num = 1, bool is_recovery = false);
bool escort_is_recovery(const char* nvm_path);
void escort_finalize();
void* escort_malloc(std::size_t size);
void escort_free(void* addr, std::size_t size);
void escort_thread_init();
void escort_thread_finalize();
void escort_begin_op();
void escort_end_op();
void escort_write_region(void* addr, std::size_t size);
void escort_set_root(const char* id, void* addr);
void* escort_get_root(const char* id);
template<class T>
T* escort_get_root(const char* id) {
  return reinterpret_cast<T*>(escort_get_root(id));
}
void escort_remove_root(const char* id);

void escort_get_current_status(int* epoch, int* phase,
			       int* num_user_threads,
			       int* num_used_alloc_logs,
			       int* num_delayed_deallocs);

#define ESCORT_CACHELINE_SIZE 64

namespace {
  template<typename T>
  void pwrite(T* addr, T val) {
    std::size_t object_size = sizeof(T);
    escort_write_region(addr, object_size);
    *addr = val;
  }
  
  template<class T, typename... Types>
  T* pnew(Types&&... args) {
    std::size_t object_size = sizeof(T);
    if(object_size % ESCORT_CACHELINE_SIZE != 0) {
      object_size = (object_size/ESCORT_CACHELINE_SIZE + 1) * ESCORT_CACHELINE_SIZE;
    }
    T* ret = (T*) escort_malloc(object_size);
    new (ret) T(std::forward<Types>(args)...);
    if((void*) ret == (void*) 0x7f9f164d9fc8)
      std::cout << "ret: " << ret << ", size: " << object_size << std::endl;
    return ret;
  }

  template<class T>
  void pdelete(T* obj) {
    obj->~T();
    std::size_t object_size = sizeof(T);
    if(object_size % ESCORT_CACHELINE_SIZE != 0) {
      object_size = (object_size/ESCORT_CACHELINE_SIZE + 1) * ESCORT_CACHELINE_SIZE;
      escort_free(obj, object_size);
    }
  }
}

#define PWRITE(var, val) ({			\
      pwrite(*var, val);})
#define PNEW(t, ...) ({				\
      pnew<t>(__VA_ARGS__);})
#define PDELETE(obj) ({				\
      pdelete(obj);})
#endif
