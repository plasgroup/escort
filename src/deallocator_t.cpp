#include "deallocator_t.hpp"
#include "userthreadctx_t.hpp"

void Escort::deallocator_t::dealloc(std::vector<userthreadctx_t*>& ctx_array) {
  for(auto ctx: ctx_array) {
    auto dealloc_list = ctx->dealloc_list();
    for(auto addr: dealloc_list) {
      escort_free(addr);
    }
  }
}
