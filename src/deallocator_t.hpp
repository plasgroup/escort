#ifndef DEALLOCATOR_T_HPP
#define DEALLOCATOR_T_HPP
#include <thread>
#include <vector>
#include "globalEscort.hpp"

#include "jemalloc/include/jemalloc/jemalloc.h"

class Escort::deallocator_t {
private:
  std::thread _thread;
private:
  void dealloc(std::vector<userthreadctx_t*>& ctx_array);
public:
  inline void run(std::vector<userthreadctx_t*>& ctx_array) {
    _thread = std::thread(&Escort::deallocator_t::dealloc, this, std::ref(ctx_array));
  }
  inline void join() { _thread.join(); }
};
#endif
