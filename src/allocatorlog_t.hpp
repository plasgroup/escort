#ifndef ALLOCATORLOG_T_HPP
#define ALLOCATORLOG_T_HPP

#include <list>
#include <vector>

#include "Escort.hpp"
#include "globalEscort.hpp"
#include "nvm/config.hpp"

#define ALLOCATOR_LOG_BLOCK_NUM 1024

namespace gv = Escort::GlobalVariable;

class Escort::allocatorlog_t {
public:
  class log_block_t {
  public:
    class entry_t {
    public:
      void* addr;
      std::size_t size;
      bool is_valid;
    };
  private:
    std::size_t _size;
    std::size_t _max_size;
    epoch_t _epoch;
  public:
    entry_t _entries[ALLOCATOR_LOG_BLOCK_NUM];
  public:
    log_block_t():_size(0), _max_size(ALLOCATOR_LOG_BLOCK_NUM) {}
    inline std::size_t size() const { return _size; }
    inline epoch_t epoch() const { return _epoch; }
    inline void set_epoch(epoch_t epoch){ _epoch = epoch; }
    inline const entry_t* entries() const { return reinterpret_cast<const entry_t*>(&_entries); }
    void flush() {
      _mm_clwb(&_size);
      if(CACHE_LINE_SIZE >= sizeof(entry_t)) {
	int clwb_window = CACHE_LINE_SIZE / sizeof(entry_t);
	for(int i = 0; i < _size; i+= clwb_window) 
	  _mm_clwb(&_entries[i]);
      } else {
	// under construction
	// this implementaion must not reach here.
	DEBUG_ERROR("must not reach here", "entry_t size", sizeof(entry_t));
      }
    }
    
    bool append(std::pair<void*, std::size_t> entry) {
      if(_size == ALLOCATOR_LOG_BLOCK_NUM)
	return false;
      
      _entries[_size].addr     = entry.first;
      _entries[_size].size     = entry.second;
      _entries[_size].is_valid = true;
      
      _size++;
      return true;
    }
    
    void clear() {
      _size = 0;
      _mm_clwb(&_size);
    }
  };
private:
  std::list<log_block_t*> _block_list;
public:
  void append(std::pair<void*, std::size_t> entry, epoch_t epoch);
  void clear();
  inline const std::list<log_block_t*>& block_list() const {
    return _block_list;
  }
};

using allocatorlog_block_t = Escort::allocatorlog_t::log_block_t;

class Escort::allocatorlog_management_t {
private:
  std::mutex _mtx;
  
  std::list<allocatorlog_block_t*> _free_block_list;
  std::list<allocatorlog_block_t*> _used_block_list;
public:
  allocatorlog_management_t(size_t num_blocks = 1024) {
    std::intptr_t log_area = reinterpret_cast<std::intptr_t>(gv::NVM_config->allocatorlog_area());
    
    for(int i = 0; i < num_blocks; i++) {
      allocatorlog_block_t* block = reinterpret_cast<allocatorlog_block_t*>(log_area);
      new(block) allocatorlog_block_t();
      _free_block_list.push_back(reinterpret_cast<allocatorlog_block_t*>(log_area));
      log_area += sizeof(allocatorlog_block_t);
    }
  }

  inline allocatorlog_block_t* pop() {
    std::lock_guard<std::mutex> lock(_mtx);
    assert(_free_block_list.size() > 0);
    allocatorlog_block_t* log_block = _free_block_list.back();
    _free_block_list.pop_back();
    _used_block_list.push_back(log_block);
    return log_block;
  }
  
  inline void push(allocatorlog_block_t* log_block) {
    std::lock_guard<std::mutex> lock(_mtx);
    _free_block_list.push_back(log_block);
    _used_block_list.remove(log_block);
  }

  inline const std::list<allocatorlog_block_t*>& used_list() const {
    return _used_block_list;
  }
};

#endif
