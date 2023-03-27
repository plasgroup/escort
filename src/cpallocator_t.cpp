#include "cpallocator_t.hpp"
#include "allocatorlog_t.hpp"
#include "userthreadctx_t.hpp"
#include "bytemap_t.hpp"

namespace gv = Escort::GlobalVariable;

void Escort::cpallocator_t::check_valid(std::vector<userthreadctx_t*>& ctx_array) {
  auto epoch = GLOBAL_EPOCH;
  auto prev  = static_cast<bool>(epoch & 0x1);
  for(auto ctx: ctx_array) {
    auto allocatorlog = ctx->allocatorlog(prev);
    auto block_list  = allocatorlog.block_list();
    for(auto block: block_list) {
      const std::size_t size = block->size();
      auto entries = block->entries();
      for(int i = 0; i < size; i++) {
        auto& entry = entries[i];
	const auto index = gv::ByteMap[prev]->get_index(entry.addr);
	if(!gv::ByteMap[prev]->is_set(index)) {
	  block->_entries[i].is_valid = false;
	  // entry.is_valid = false; 
	}
	gv::ByteMap[prev]->clear(index);
      }
    }
  }
}

void Escort::cpallocator_t::checkpointing(std::vector<userthreadctx_t*>& ctx_array) {
  auto epoch = GLOBAL_EPOCH;
  auto prev  = static_cast<bool>(epoch & 0x1);
  for(auto ctx: ctx_array) {
    auto allocatorlog = ctx->allocatorlog(prev);
    auto block_list = allocatorlog.block_list();
    for(auto block: block_list) {
      const std::size_t size = block->size();
      auto entries = block->entries();
      for(int i = 0; i < size; i++) {
	auto& entry = entries[i];
	auto index = gv::ByteMap[prev]->get_index(entry.addr);
	gv::ByteMap_NVM->set_obj_size(index, entry.size);
	gv::ByteMap_NVM->clwb(index);
      }
    }
    _mm_sfence();
    allocatorlog.clear();
  }
  _mm_sfence();
}

