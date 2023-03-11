#ifndef BYTEMAP_T_HPP
#define BYTEMAP_T_HPP

#include "Escort.hpp"
#include "globalEscort.hpp"
#include "debug.hpp"
#include "nvm/config.hpp"

namespace gv = Escort::GlobalVariable;

using byte = char;

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
    assert(!_bytemap[index]); // this assertion works when program is DRF
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

  // this size is not real size
  // size means real size / CACHE_LINE_SIZE
  inline void set(std::size_t index, std::size_t size) {
    _bytemap[index] = size;
  }
  inline void clwb(std::size_t index) {
    _mm_clwb(&_bytemap[index]);
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
