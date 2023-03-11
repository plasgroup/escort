#ifndef GLOBALESCORT_HPP
#define GLOBALESCORT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <iostream>

namespace Escort {
  class nvmconfig_t;
  class userthreadctx_t;
  class bitmap_t;
  class bytemap_t;
  class bytemap_dram_t;
  class bytemap_nvm_t;
  class roottable_t;
  class plog_t;
  class allocatorlog_t;
  class addrlist_t;
  class cacheline_t;
  class allocatorlog_management_t;
  class plog_management_t;
  class cpmaster_t;
  class deallocator_t;
  class cpallocator_t;
  
  using epoch_t = std::uint64_t;
  
  extern std::uint32_t LOG_CACHE_LINE_SIZE;
  // extern std::uint32_t CACHE_LINE_SIZE;
  extern std::size_t HEAP_SIZE;
  extern std::intptr_t DRAM_BASE;
  
  namespace GlobalVariable {
    extern nvmconfig_t *NVM_config;
    extern epoch_t *Epoch; // use GLOBALEPOCH (defined below)
    extern bitmap_t *BitMap[2];
    extern bytemap_dram_t *ByteMap[2];
    extern bytemap_nvm_t *ByteMap_NVM;
    extern roottable_t *RootTable;
    extern bool isPersisting;
    extern std::uint32_t EpochLength;
    extern cpmaster_t *CpMaster;
    extern plog_management_t *Plog_Management;
    extern allocatorlog_management_t *Allocatorlog_Management;
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
}

#define GLOBAL_EPOCH *(Escort::GlobalVariable::Epoch)

#define NEW(t, ...) ({				\
      Escort::default_new<t>(__VA_ARGS__);})
#define DELETE(obj) ({				\
      Escort::default_delete(obj);})

#endif
