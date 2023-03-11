// This is the only file the user can see.
#ifndef ESCORT_HPP
#define ESCORT_HPP

#include <cstddef>
#include <cstdint>
#include <iostream>

void Escort_init(const char* nvm_path, std::uint32_t checkpointing_num = 1);
void Escort_finalize();

void* Escort_malloc(std::size_t size);
void Escort_free(void* addr, std::size_t size);
void Escort_thread_init();
void Escort_thread_finalize();
void Escort_begin_op();
void Escort_end_op();
void Escort_write_region(void* addr, std::size_t size);
void Escort_set_root(const char* id, void* addr);
void* Escort_get_root(const char* id);
template<class T>
T* Escort_get_root(const char* id) {
  return reinterpret_cast<T*>(Escort_get_root(id));
}
void Escort_remove_root(const char* id);

#define CACHE_LINE_SIZE 64

namespace {
  template<class T, typename... Types>
  T* pnew(Types... args) {
    std::size_t object_size = sizeof(T);
    if(object_size % CACHE_LINE_SIZE != 0) {
      object_size = (object_size/CACHE_LINE_SIZE + 1) * CACHE_LINE_SIZE;
    }
    T* ret = (T*) Escort_malloc(object_size);
    new (ret) T(args...);
    if((void*) ret == (void*) 0x7f9f164d9fc8)
      std::cout << "ret: " << ret << ", size: " << object_size << std::endl;
    return ret;
  }

  template<class T>
  void pdelete(T* obj) {
    obj->~T();
    std::size_t object_size = sizeof(T);
    if(object_size % CACHE_LINE_SIZE != 0) {
      object_size = (object_size/CACHE_LINE_SIZE + 1) * CACHE_LINE_SIZE;
      Escort_free(obj, object_size);
    }
  }
}
//
#define ESCORT_WRITE()
#define PNEW(t, ...) ({ \
      pnew<t>(__VA_ARGS__);})
#define PDELETE(obj) ({ \
      pdelete(obj);})

#endif
