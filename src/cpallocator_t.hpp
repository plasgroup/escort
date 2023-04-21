#ifdef SAVE_ALLOCATOR

#ifndef CPALLOCATOR_T_HPP
#define CPALLOCATOR_T_HPP

#include <vector>
#include <thread>

#include "globalEscort.hpp"
#include "userthreadctx_t.hpp"

class Escort::cpallocator_t {
private:
  std::thread _thread;
private:
  void check_valid(std::vector<userthreadctx_t*>& ctx_array);
  void checkpointing(std::vector<userthreadctx_t*>& ctx_array);
public:
  inline void run_check_valid(std::vector<userthreadctx_t*>& ctx_array) { _thread = std::thread(&Escort::cpallocator_t::check_valid, this, std::ref(ctx_array)); }
  inline void run_checkpointing(std::vector<userthreadctx_t*>& ctx_array) { _thread = std::thread(&Escort::cpallocator_t::checkpointing, this, std::ref(ctx_array)); }
  inline void join() { _thread.join(); }
  
};
#endif

#endif /* SAVE_ALLOCATOR */
