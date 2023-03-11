#ifndef LOG_HPP
#define LOG_HPP

#include "globalEscort.hpp"
#include "nvm/allocator.hpp"

#define BLOCK_SIZE 1024

namespace Escort {
  class plog_t {
  public:
    class entry_t {
      void* _addr;
      char _val[CACHE_LINE_SIZE];
    public:
      inline void set(void* addr) {
	_addr = addr;
	_mm_clwb(_addr);
	memcpy(_val, addr, CACHE_LINE_SIZE);
	_mm_clwb(_val);
      }
      inline void set_addr(void* addr) {
	_addr = addr;
	_mm_clwb(_addr);
      }
      inline void get_addr() const { return _addr; }
    };
  private:
    size_t _size;
    size_t _max_size;
    entry_t* _head, _tail;
  public:
    Log() {
      _size = 0;
      _max_size = BLOCK_SIZE;
      _head = NVM::malloc(sizeof(Entry)*(BLOCK_SIZE+1));
      _tail = _head;
      _mm_clwb(&_size);
    }

    void append(void* dram_heap_addr) {
      if(_size % BLOCK_SIZE == 0 && _size != 0) { // move next array
	if(_size == _max_size) { // create next array
	  entry_t *next_array = NVM::malloc(sizeof(Entry)*(BLOCK_SIZE+1));
	  _tail->set_addr(next_addr);
	}
	_tail = _tail->get_addr(); 
      }
      
      _tail->set(addr);
      _tail = add_addr(_tail, addr);
      _size += 1;
    }
    
  };
}

#endif
