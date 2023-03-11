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
	const auto index = gv::ByteMap[prev]->get_index(entry.addr);
	set_size_bytemap(index, entry.size);
      }
    }
  }
  _mm_sfence();
}

void Escort::cpallocator_t::set_size_bytemap(std::size_t index, std::size_t size) {
  std::size_t  tmp_index = index;
  std::int64_t tmp_size  = size/CACHE_LINE_SIZE;
  
  for(; tmp_size > 0; tmp_size -= CACHE_LINE_SIZE) {
    if(tmp_size >= CACHE_LINE_SIZE)
      gv::ByteMap_NVM->set(tmp_index, CACHE_LINE_SIZE);
    else
      gv::ByteMap_NVM->set(tmp_index, tmp_size);
    tmp_index++;
  }

  for(int loop_count = 0; loop_count < size/CACHE_LINE_SIZE; loop_count += CACHE_LINE_SIZE) {
    gv::ByteMap_NVM->clwb(index+loop_count);
  }
}
