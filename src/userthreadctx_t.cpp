#include <cassert>

#include "userthreadctx_t.hpp"
#include "debug.hpp"
#include "Escort.hpp"
#include "bitmap_t.hpp"
#include "bytemap_t.hpp"
#include "cacheline_t.hpp"
#include "roottable_t.hpp"

#include "jemalloc/include/jemalloc/jemalloc.h"

using namespace Escort;
namespace gv = Escort::GlobalVariable;

inline void* userthreadctx_t::malloc_with_create_log(std::size_t size) {
  bool is_out_of_transaction = false;
  epoch_t epoch = get_epoch(std::memory_order_relaxed);
  if(epoch == 0) {
    is_out_of_transaction = true;
    begin_op();
  }

  void *ret = escort_mallocx(size, MALLOCX_ARENA(1)|MALLOCX_TCACHE_NONE);

  bool curr = static_cast<bool>(epoch & 0x1);
  std::size_t index = gv::ByteMap[curr]->get_index(ret);
  if(gv::ByteMap[curr]->is_set(index)) {
    gv::ByteMap[curr]->reverse(index);
  } else {
    gv::ByteMap[curr]->set(index);
    _allocatorlog[curr]->append({ret, size}, epoch);
  }
  
  if(is_out_of_transaction) {
    end_op();
  }
  return ret;
}

inline void userthreadctx_t::free_with_create_log(void* addr, std::size_t size) {
  bool is_out_of_transaction = false;
  epoch_t epoch = get_epoch(std::memory_order_relaxed);
  if(epoch == 0) {
    is_out_of_transaction = true;
    begin_op();
  }

  bool curr = static_cast<bool>(epoch & 0x1);
  std::size_t index = gv::ByteMap[curr]->get_index(addr);

  // this bytemap check should be executed
  // before real free oparation
  // for preventing from multiple creation
  // of allocator log
  if(gv::ByteMap[curr]->is_set(index)) {
    gv::ByteMap[curr]->reverse(index);
  } else {
    gv::ByteMap[curr]->set(index);
    _allocatorlog[curr]->append({addr, size}, epoch);
  }

  // this if statement considers that
  // free operation at current epoch is executed
  // before malloc operation at previous epoch,
  // and malloc operation get memory space that is
  // deallocated by free operation at current epoch
  if(get_phase(epoch) ==
     epoch_phase::Multi_Epoch_Existence) { // must delay
    DEBUG_PRINT("free operation delays");
    _delay_dealloc_list.push_back(addr);
  } else {
    escort_free(addr);
  }
	      
  if(is_out_of_transaction) {
    end_op();
  }
}

void* userthreadctx_t::malloc(std::size_t size){
#ifdef NO_PERSIST_METADATA
  return escort_mallocx(size, MALLOCX_ARENA(1));
#else
  return malloc_with_create_log(size);
#endif
}
void userthreadctx_t::free(void* addr, std::size_t size) {
#ifdef NO_PERSIST_METADATA
  escort_free(addr);
#else
  free_with_create_log(addr, size);
#endif
}

void userthreadctx_t::begin_op() {
#ifdef WEAK_CONSISTENCY
  set_epoch(GLOBAL_EPOCH, std::memory_order_relaxed);
#else
  do {
    set_epoch(GLOBAL_EPOCH, std::memory_order_seq_cst);
  } while(get_epoch(std::memory_order_relaxed) != GLOBAL_EPOCH);
#endif
}

void userthreadctx_t::end_op() {
  assert(get_epoch(std::memory_order_relaxed) != 0);
  set_epoch(0, std::memory_order_relaxed);
}
void userthreadctx_t::mark(void* addr, std::size_t size) {
  std::uint64_t epoch = get_epoch(std::memory_order_relaxed);
  assert(epoch != 0);
  
  bool curr = static_cast<bool>(epoch & 0x1);
  bool prev = (!curr);

  std::size_t index = gv::BitMap[curr]->get_index(addr);
  std::size_t block_size = (size + CACHE_LINE_SIZE - 1) >> LOG_CACHE_LINE_SIZE;
  std::size_t end_index = block_size + index;
  
  bool is_append_list = false;
  auto addr_ptr = reinterpret_cast<std::intptr_t>(addr);
  addr_ptr -= addr_ptr%CACHE_LINE_SIZE;
  addr = reinterpret_cast<void*>(addr_ptr);
  std::pair<void*, std::size_t> addrlist_entry = {addr, block_size};
  for(; index < end_index; index++) {
    if(!gv::BitMap[curr]->is_set(index)) {
      if(gv::BitMap[prev]->is_set(index)) {
	cacheline_t &old_val = *(reinterpret_cast<cacheline_t*>(addr));
	gv::BitMap[prev]->clear(index);
	_log->push_back_and_clwb({reinterpret_cast<cacheline_t*>(addr), old_val});
      }
      gv::BitMap[curr]->set(index);
      is_append_list = true;
    }
    addr = add_addr(addr, CACHE_LINE_SIZE);
  }
  if(is_append_list) 
    _addrlist[curr]->append(addrlist_entry);
}

void userthreadctx_t::set_root(const char* id, void* addr) {
  assert(get_epoch(std::memory_order_relaxed) != 0);
  gv::RootTable->set(id, addr);
}
void* userthreadctx_t::get_root(const char* id) {
  assert(get_epoch(std::memory_order_relaxed) != 0);
  return gv::RootTable->get(id);
}
void userthreadctx_t::remove_root(const char* id) {
  assert(get_epoch(std::memory_order_relaxed) != 0);
  return gv::RootTable->remove(id);
}
