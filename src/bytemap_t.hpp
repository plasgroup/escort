#ifndef BYTEMAP_T_HPP
#define BYTEMAP_T_HPP

#include "Escort.hpp"
#include "globalEscort.hpp"
#include "debug.hpp"
#include "nvm/config.hpp"

namespace gv = Escort::GlobalVariable;

using byte = char;
#define MAX_BYTE 64
class Escort::bytemap_t {
protected:
  byte* _bytemap;
public:
  bytemap_t(std::size_t arraysize = Escort::HEAP_SIZE) {}

  inline std::size_t get_index(void* addr) const {
    std::intptr_t diff = diff_addr(addr);
    assert(diff >= 0);
    return static_cast<size_t>(diff / CACHE_LINE_SIZE);
  }

  inline void clear(std::size_t index) {
    _bytemap[index] = 0;
  }

  inline byte get_byte(std::size_t index) const {
    return  _bytemap[index];
  }
};

class Escort::bytemap_dram_t: public Escort::bytemap_t {
public:
  bytemap_dram_t(std::size_t arraysize = Escort::HEAP_SIZE) {
    _bytemap = (byte*) std::malloc(arraysize * sizeof(byte)/CACHE_LINE_SIZE);
  }

  inline bool is_set(std::size_t index) const {
    return (_bytemap[index] == 1);
  }
  inline void set(std::size_t index) {
    // assert(!_bytemap[index]); // this assertion works when program is DRF
    _bytemap[index]  =  1;
  }
  inline void reverse(std::size_t index) {
    _bytemap[index] *= -1;
  }
};

class Escort::bytemap_nvm_t: public Escort::bytemap_t {
public:
  bytemap_nvm_t(std::size_t arraysize = Escort::HEAP_SIZE) {
    _bytemap = (byte*) gv::NVM_config->malloc(arraysize * sizeof(byte)/CACHE_LINE_SIZE);
  }

  // num_bytes means real size / CACHE_LINE_SIZE
  inline void set(std::size_t index, std::size_t num_bytes) {
    _bytemap[index] = num_bytes;
  }
  inline void clwb(std::size_t index) {
    _mm_clwb(&_bytemap[index]);
  }

  std::pair<std::size_t, std::size_t> get_obj_size(std::size_t index) {
    return {};
  }
  void set_obj_size(std::size_t index, std::size_t obj_size) {
    const std::size_t arraysize = Escort::HEAP_SIZE * sizeof(byte) / CACHE_LINE_SIZE;
    std::size_t obj_bytes_sum = obj_size / CACHE_LINE_SIZE;

    std::size_t debug_block_array_num = (obj_bytes_sum + CACHE_LINE_SIZE - 1) >> LOG_CACHE_LINE_SIZE;    
    assert(index+debug_block_array_num <= arraysize);

    while(obj_bytes_sum > 0) {
      if(obj_bytes_sum >= CACHE_LINE_SIZE) {
	_bytemap[index] = CACHE_LINE_SIZE;
	obj_bytes_sum -= CACHE_LINE_SIZE;
      } else {
	_bytemap[index] = obj_bytes_sum;
	obj_bytes_sum = 0;
      }
      index++;
    }
  }
  
#ifdef DEBUG_MODE
  void debug_print() {
    for(std::size_t i = 0; i < Escort::HEAP_SIZE/CACHE_LINE_SIZE; i++) {
      if(_bytemap[i] != 0)
	DEBUG_PRINT("i:", i, ", val:", _bytemap[i]);
    }
  }
#endif
};
#endif
