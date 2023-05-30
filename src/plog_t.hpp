#ifndef PLOG_T_HPP
#define PLOG_T_HPP

#include <x86intrin.h>
#include <assert.h>
#include <cstring>
#include <vector>
#include <list>
#include <mutex>

#include "globalEscort.hpp"
#include "cacheline_t.hpp"
#include "debug.hpp"
#include "nvm/config.hpp"

#define LOG_BLOCK_NUM (4096*8)

namespace gv = Escort::GlobalVariable;

#ifndef OLD_VERSION
class Escort::plog_t {
public:
  class log_block_t {
  public:
    class entry_t {
    public:
      alignas(CACHE_LINE_SIZE) cacheline_t* addr;
      alignas(CACHE_LINE_SIZE) cacheline_t val;
    };
  private:
    entry_t _entries[LOG_BLOCK_NUM];
    std::size_t _size;
    std::size_t _max_size;
  public:
    log_block_t(): _size(0), _max_size(LOG_BLOCK_NUM) {}
    inline std::size_t size() const { return _size; }
    inline const entry_t* entries() const { return reinterpret_cast<const entry_t*>(&_entries); }
    inline bool append(std::pair<cacheline_t*, cacheline_t&> entry) {
      if(_size > LOG_BLOCK_NUM)
	DEBUG_ERROR("plog size error:", _size);
      if(_size == LOG_BLOCK_NUM)
	return false;
      
      _entries[_size].addr = entry.first;
      _mm_clwb(&_entries[_size].addr);
      _entries[_size].val = entry.second;
      _mm_clwb(&_entries[_size].val);
      _size++;
      return true;
    }
    inline void flush() { _mm_clwb(&_size); }
    inline void clear() {
      _size = 0;
      _mm_clwb(&_size);
    }
  };
private:
  std::list<log_block_t*> _block_list;
public:
  plog_t() {}
  void push_back_and_clwb(std::pair<cacheline_t*, cacheline_t&> entry);
  void flush();
  void clear();
  inline const std::list<log_block_t*>& block_list() const {
    return _block_list;
  }
};

using log_block_t = Escort::plog_t::log_block_t;

class Escort::plog_management_t {
private:
  std::mutex _mtx;
  std::list<log_block_t*> _free_block_list;
  std::list<log_block_t*> _used_block_list;
public:
#ifdef ALLOCATOR_RALLOC
  plog_management_t(uintptr_t plog_area_addr, size_t plog_area_size) {
    int num_blocks = plog_area_size / sizeof(log_block_t);
    uintptr_t p = plog_area_addr;
    for (int i = 0; i < num_blocks; i++) {
      log_block_t* block = reinterpret_cast<log_block_t*>(p);
      new(block) log_block_t();
      _free_block_list.push_back(block);
      p += sizeof(log_block_t);
      p = (p + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);
    }
  }
#else // ALLOCATOR_RALLOC
  plog_management_t(size_t num_blocks = 1024) {
    std::intptr_t log_area = reinterpret_cast<std::intptr_t>(gv::NVM_config->redolog_area());
    for(int i = 0; i < num_blocks; i++) {
      log_block_t* block = reinterpret_cast<log_block_t*>(log_area);
      new(block) log_block_t();
      _free_block_list.push_back(reinterpret_cast<log_block_t*>(log_area));
      log_area += sizeof(log_block_t);
    }
  }
#endif // ALLOCATOR_RALLOC

  inline log_block_t* pop() {
    std::lock_guard<std::mutex> lock(_mtx);
    assert(_free_block_list.size() > 0);
    log_block_t* log_block = _free_block_list.back();
    _free_block_list.pop_back();
    _used_block_list.push_back(log_block);
    return log_block;
  }

  inline void push(log_block_t* log_block) {
    std::lock_guard<std::mutex> lock(_mtx);
    _free_block_list.push_back(log_block);
    _used_block_list.remove(log_block);
  }

  inline const std::list<log_block_t*>& free_list() const {
    return _free_block_list;
  }
  inline const std::list<log_block_t*>& used_list() const {
    return _used_block_list;
  }
};
#else
class Escort::plog_t {
public:
  class entry_t {
  public:
    cacheline_t* addr;
    cacheline_t  val;
  public:
    entry_t() {}
    inline void set(std::pair<cacheline_t*, cacheline_t&> entry) {
      addr = entry.first;
      _mm_clwb(&addr);
      val  = entry.second;
      _mm_clwb(&val);
    }
    void clear() {
      addr = nullptr; val = 0;
      _mm_clwb(&addr);
      _mm_clwb(&val);
    }
  };

private:
  entry_t _array[LOG_SIZE];
  std::size_t _size;
  std::size_t _max_size;
public:
  plog_t() :_size(0), _max_size(LOG_SIZE){}
  void push_back_and_clwb(std::pair<cacheline_t*, cacheline_t&> entry);
  entry_t& pop();
  inline bool is_empty() const { return _size == 0; }
  void flush();
  void clear();
  const std::size_t size() const { return _size; }
};

class Escort::plog_management_t{}; // do not use this class object
#endif
#endif
