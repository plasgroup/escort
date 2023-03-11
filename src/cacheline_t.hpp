#ifndef CACHELINE_T_HPP
#define CACHELINE_T_HPP

#include <cstring>
#include "Escort.hpp"

using byte = char;

class Escort::cacheline_t {
private:
  byte _content[CACHE_LINE_SIZE];
public:
  cacheline_t() {}
  ~cacheline_t() {}
  const byte* get_content() const { return _content; }
  cacheline_t &operator =(const cacheline_t &cacheline) {
    memcpy(_content, cacheline.get_content(), CACHE_LINE_SIZE);
    return *this;
  }
};
#endif
