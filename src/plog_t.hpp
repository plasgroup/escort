#ifndef PLOG_T_HPP
#define PLOG_T_HPP

#include <cstring>
#include <vector>
#include <list>

#include "globalEscort.hpp"
#include "cacheline_t.hpp"
#include "debug.hpp"
#include "nvm/config.hpp"

#define LOG_BLOCK_NUM 4096

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
    std::size_t _size;
    std::size_t _max_size;
    entry_t _entries[LOG_BLOCK_NUM];
  public:
    log_block_t(): _size(0), _max_size(LOG_BLOCK_NUM) {}
    inline std::size_t size() const { return _size; }
    inline const entry_t* entries() const { return reinterpret_cast<const entry_t*>(&_entries); }
    inline bool append(std::pair<cacheline_t*, cacheline_t&> entry) {
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
  plog_management_t(size_t num_blocks = 4096) {
    std::intptr_t log_area = reinterpret_cast<std::intptr_t>(gv::NVM_config->redolog_area());
    for(int i = 0; i < num_blocks; i++) {
      log_block_t* block = reinterpret_cast<log_block_t*>(log_area);
      new(block) log_block_t();
      _free_block_list.push_back(reinterpret_cast<log_block_t*>(log_area));
      log_area += sizeof(log_block_t);
    }
  }

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
};

class Escort::plog_management_t{}; // do not use this class object
#endif
#endif
