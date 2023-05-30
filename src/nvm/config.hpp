#ifndef NVM_CONFIG_HPP
#define NVM_CONFIG_HPP

#ifndef ALLOCATOR_RALLOC

#include <filesystem>
#include <fstream>
#include <climits>
#include <vector>
#include <mutex>
#include <cassert>

#include <sys/mman.h>
#include <fcntl.h>
#include <x86intrin.h>

#include "../globalEscort.hpp"
#include "../debug.hpp"

#ifdef NVM
#define MMAP_FLAG MAP_SHARED
#else
#define MMAP_FLAG MAP_SHARED
#endif


namespace Escort {
  class nvmconfig_t {
    // |_dram_space|_nvm_space|_g_epoch_pointer|_bytemap_pointer|_root_pointer|nvm_heap area|persist_num|user_num|
  private:
    std::intptr_t _dram_space; // start address of dram heap
    std::intptr_t _nvm_space; // start address of nvm heap
    std::intptr_t _g_epoch_pointer; // global epoch pointer
    std::intptr_t _bytemap_pointer; // bytemap pointer
    std::intptr_t _root_pointer; // root pointer
    std::intptr_t _tmp_space;

    void**          _dram_space_addr;
    void**          _nvm_space_addr;
    bytemap_nvm_t** _bytemap_pointer_addr;
    epoch_t*        _g_epoch_pointer_addr;    
    roottable_t**   _root_pointer_addr;
    std::uint64_t*  _persist_num_addr;
    std::uint64_t*  _user_num_addr;
    
    std::size_t _nvm_size;
    std::size_t _redolog_area_size;
#ifdef SAVE_ALLOCATOR
    std::size_t _allocatorlog_area_size;
#endif /* SAVE_ALLOCATOR */
    int _nvm_fd;
    int _nvm_redolog_fd;
#ifdef SAVE_ALLOCATOR
    int _nvm_allocatorlog_fd;
#endif /* SAVE_ALLOCATOR */
    void* _start_address;        // for unmap
    void* _redolog_address;       // for unmap
#ifdef SAVE_ALLOCATOR
    void* _allocatorlog_address; // for unmap
#endif /* SAVE_ALLOCATOR */

    std::mutex _mtx_malloc;
    bool _is_dirty; // for recovery
  private:
    void recovery_heap() {} // under construction
    void create_area(std::size_t heap_size);
    void create_redolog_area(const char* redolog_path);
#ifdef SAVE_ALLOCATOR
    void create_allocatorlog_area(const char* allocatorlog_path);
#endif /* SAVE_ALLOCATOR */
    void check_dirty();
    std::intptr_t open_nvmfile(const char* nvm_path);
  public:
    nvmconfig_t() {} 
    nvmconfig_t(const char* nvm_path, std::size_t heap_size = Escort::HEAP_SIZE);
    ~nvmconfig_t();
    
    void* dram_space() const {
      return reinterpret_cast<void*>(_dram_space);
    }
    void* nvm_space() const {
      return reinterpret_cast<void*>(_nvm_space);
    }
    inline std::int64_t offset() const {
      return static_cast<std::int64_t>(_nvm_space - _dram_space);  
    }

    inline void* malloc(std::size_t size) { // for bytemap
      std::lock_guard<std::mutex> lock(_mtx_malloc);
      std::size_t whole_size  = _tmp_space - (std::intptr_t) _start_address;
      assert(whole_size < _nvm_size);
      void* ret = reinterpret_cast<void*>(_tmp_space);
      _tmp_space += size;
      return ret;
    }

    inline void* redolog_area() const { return _redolog_address; }
#ifdef SAVE_ALLOCATOR
    inline void* allocatorlog_area() const { return _allocatorlog_address; }
#endif /* SAVE_ALLOCATOR */
    
    inline bool is_dirty() const {
      return _is_dirty;
    }

    inline void set_root_pointer(void* root_pointer) {
      _root_pointer = reinterpret_cast<std::intptr_t>(root_pointer);
      *(_root_pointer_addr) = reinterpret_cast<roottable_t*>(root_pointer);
      _mm_clwb(_root_pointer_addr);
    }

    inline void set_bytemap_pointer(void* bytemap_pointer) {
      _bytemap_pointer = reinterpret_cast<std::intptr_t>(bytemap_pointer);
      *(_bytemap_pointer_addr) = reinterpret_cast<bytemap_nvm_t*>(bytemap_pointer);
      _mm_clwb(_bytemap_pointer_addr);
    }
    
    inline roottable_t* get_roottable() {
      DEBUG_PRINT("root_pointer_addr:", _root_pointer_addr);
      return *(_root_pointer_addr);
    }
    
    inline void add_user_num() {
      std::uint64_t user_num = *_user_num_addr;
      *_user_num_addr = user_num + 1;
      _mm_clwb(_user_num_addr);
      _mm_sfence();
    }
    
    inline void flush_variable() {
      _mm_clwb(_start_address);
      _mm_sfence();
    }

    inline void reset_thread_num() {
      *_persist_num_addr = 0;
      _mm_clwb(_persist_num_addr);
      *_user_num_addr = 0;
      _mm_clwb(_user_num_addr);
    }

    inline void set_checkpointing_thread(std::uint32_t checkpointing_num) {
      *_persist_num_addr = checkpointing_num;
      _mm_clwb(_persist_num_addr);
      _mm_sfence();
    }
    
    // for persistent objects in library
    template<class T, typename... Types>
    T* pnew(Types... args) {
      T* ret = (T*) this->malloc(sizeof(T));
      new(ret) T(args...);
      return ret;
    }
  };
}
#endif // ALLOCATOR_RALLOC

#endif
