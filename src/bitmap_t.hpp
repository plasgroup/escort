#ifndef BITMAP_T_HPP
#define BITMAP_T_HPP

#include <cassert>
#include <cstddef>

#include "Escort.hpp"
#include "globalEscort.hpp"

class Escort::bitmap_t {
private:
  bool* _bitmap;
  std::size_t _size;
public:
  bitmap_t(std::size_t arraysize = Escort::HEAP_SIZE) {
    _bitmap = reinterpret_cast<bool*>(std::calloc(arraysize/CACHE_LINE_SIZE, sizeof(bool)));
    _size   = arraysize/CACHE_LINE_SIZE;
  }
  
  inline std::size_t get_index(void* addr) const {
    std::intptr_t diff = diff_addr(addr);
    assert(diff >= 0);
    return static_cast<size_t>(diff / CACHE_LINE_SIZE);
  }
  
  inline bool is_set(std::size_t index) const {
    assert(index < _size);
    return _bitmap[index];
  }
  
  inline void set(std::size_t index) {
    assert(index < _size);
    assert(!_bitmap[index]); // this assertion works when program is DRF
    _bitmap[index] = true;
  }
  
  inline void clear(std::size_t index) {
    assert(index < _size);
    _bitmap[index] = false;
  }
  
#ifdef DEBUG_MODE
  inline bool is_clear() {
    for(std::size_t i = 0; i < _size; i++)
      if(_bitmap[i]) return false;
    return true;
  }
#endif
};

#endif
