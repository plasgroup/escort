#ifndef GLOBALESCORT_HPP
#define GLOBALESCORT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <iostream>

#ifdef ALLOCATOR_RALLOC
namespace Escort {
  class PersistentMetaData;
}
#endif // ALLOCATOR_RALLOC

namespace Escort {
  class nvmconfig_t;
  class userthreadctx_t;
  class bitmap_t;
  class bytemap_t;
  class bytemap_dram_t;
  class bytemap_nvm_t;
  class roottable_t;
  class plog_t;
#ifdef SAVE_ALLOCATOR
  class allocatorlog_t;
#endif /* SAVE_ALLOCATOR */
  class addrlist_t;
  class cacheline_t;
#ifdef SAVE_ALLOCATOR
  class allocatorlog_management_t;
#endif /* SAVE_ALLOCATOR */
  class plog_management_t;
  class cpmaster_t;
  class deallocator_t;
#ifdef SAVE_ALLOCATOR
  class cpallocator_t;
#endif /* SAVE_ALLOCATOR */
  
  using epoch_t = std::uint64_t;
  
  constexpr std::uint64_t LOG_CACHE_LINE_SIZE = 6;
  constexpr std::uint64_t CACHE_LINE_SIZE = (1L << LOG_CACHE_LINE_SIZE);
  extern std::size_t HEAP_SIZE;
  extern std::intptr_t DRAM_BASE;

#ifdef ALLOCATOR_RALLOC  
  extern PersistentMetaData* persistent_metadata;
#endif // ALLOCATOR_RALLOC
  namespace GlobalVariable {
#ifndef ALLOCATOR_RALLOC
    extern nvmconfig_t *NVM_config;
    extern epoch_t *Epoch; // use GLOBALEPOCH (defined below)
#endif // ALLOCATOR_RALLOC
    extern bitmap_t *BitMap[2];
#ifndef ALLOCATOR_RALLOC
    extern bytemap_dram_t *ByteMap[2];
    extern bytemap_nvm_t *ByteMap_NVM;
    extern roottable_t *RootTable;
#endif // ALLOCATOR_RALLOC
    extern bool isPersisting;
    extern std::uint32_t EpochLength;
    extern cpmaster_t *CpMaster;
    extern plog_management_t *Plog_Management;
#ifdef SAVE_ALLOCATOR
    extern allocatorlog_management_t *Allocatorlog_Management;
#endif /* SAVE_ALLOCATOR */
  }  
  namespace LocalVariable {
    extern thread_local userthreadctx_t *ctx;
  }

  enum class epoch_phase {
			  Multi_Epoch_Existence,
			  Log_Persistence,
			  NVM_Heap_Update,
			  Default
  };

  epoch_t Epoch(epoch_t epoch);
  epoch_t set_phase(epoch_t epoch, epoch_phase phase);
  epoch_phase get_phase(epoch_t epoch);
  
  constexpr std::size_t calc_heapsize(std::size_t a) { // return a GiB
    return a * (1 << 30);
  }
  
  template<typename T>
  inline T* add_addr(T* addr, std::size_t offset) {
    return reinterpret_cast<T*>(reinterpret_cast<std::intptr_t>(addr) + offset);
  }

  template<typename T>
  inline std::intptr_t diff_addr(T* addr) {
    return reinterpret_cast<std::intptr_t>(addr) - DRAM_BASE;
  }

#ifdef ALLOCATOR_RALLOC
  // defined in metadata.hpp
#else // ALLOCATOR_RALLOC
  template <class T, typename... Types>
  T* default_new(Types... args) {
    T* ret = reinterpret_cast<T*>(std::malloc(sizeof(T)));
    new (ret) T (args...);
    return ret;
  }

  template <class T>
  void default_delete(T* obj) {
    obj->~T();
    std::free(obj);
  }

  epoch_t get_global_epoch() {
    return *Escort::GlobalVariable::Epoch;
  }

  void set_global_epoch(epoch_t epoch) {
    *Escort::GlobalVariable::Epoch = epoch;
  }
#endif // ALLOCATOR_RALLOC
}

#ifdef OLD_VERSION
#define LOG_SIZE 4096*1024*4
#define MALLOC_FLAG    1
#define FREE_FLAG      0
#define INVALID_ENTRY -1
#endif

#ifdef ALLOCATOR_RALLOC
#define NEW(t, ...) new t(__VA_ARGS__)
#define DELETE(obj) delete obj
#else // ALLOCATOR_RALLOC
#define NEW(t, ...) ({				\
      Escort::default_new<t>(__VA_ARGS__);})
#define DELETE(obj) ({				\
      Escort::default_delete(obj);})
#endif // ALLOCATOR_RALLOC

#endif
