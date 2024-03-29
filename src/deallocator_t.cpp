#include "../config.h"

#include "globalEscort.hpp"
#include "deallocator_t.hpp"
#include "userthreadctx_t.hpp"

void Escort::deallocator_t::dealloc(std::vector<userthreadctx_t*>& ctx_array) {
  auto prev = static_cast<bool>((Epoch(GLOBAL_EPOCH) - 1) & 0x1);
  for(auto ctx: ctx_array) {
    auto& dealloc_list = ctx->dealloc_list(prev);
    for(auto addr: dealloc_list) {
      _escort_internal_free(addr);
    }
    ctx->clear_dealloc_list(prev);
  }
}
