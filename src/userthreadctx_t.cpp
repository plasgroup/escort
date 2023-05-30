#include <cassert>

#include "metadata.hpp"
#include "allocator.hpp"

#include "../config.h"

#include "userthreadctx_t.hpp"
#include "debug.hpp"
#include "Escort.hpp"
#include "bitmap_t.hpp"
#include "bytemap_t.hpp"
#include "cacheline_t.hpp"
#include "roottable_t.hpp"

using namespace Escort;
namespace gv = Escort::GlobalVariable;

inline void* userthreadctx_t::malloc_with_create_log(std::size_t size) {
#ifdef SAVE_ALLOCATOR
  bool is_out_of_transaction = false;
  epoch_t epoch = get_epoch(std::memory_order_relaxed);
  if(epoch == 0) {
    is_out_of_transaction = true;
    begin_op();
    epoch = get_epoch(std::memory_order_relaxed);
  }

  // void* ret = _escort_internal_mallocx(size, MALLOCX_ARENA(1));
  void* ret = _escort_internal_malloc(size);
  
  bool curr = static_cast<bool>(epoch & 0x1);
  std::size_t index = gv::ByteMap[curr]->get_index(ret);
  if(gv::ByteMap[curr]->is_set(index)) {
    gv::ByteMap[curr]->reverse(index);
  } else {
    gv::ByteMap[curr]->set(index);
    _allocatorlog[curr]->append({ret, size}, Epoch(epoch));
  }
  
  if(is_out_of_transaction) {
    end_op();
  }
  return ret;
#else /* SAVE_ALLOCATOR */
  return pm_malloc(size);
#endif /* SAVE_ALLOCATOR */
}

inline void userthreadctx_t::free_with_create_log(void* addr, std::size_t size) {
#ifdef SAVE_ALLOCATOR
  bool is_out_of_transaction = false;
  epoch_t epoch = get_epoch(std::memory_order_relaxed);
  if(epoch == 0) {
    is_out_of_transaction = true;
    begin_op();
    epoch = get_epoch(std::memory_order_relaxed);
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
    _allocatorlog[curr]->append({addr, size}, Epoch(epoch));
  }
  
  // this if statement considers that
  // free operation at current epoch is executed
  // before malloc operation at previous epoch,
  // and malloc operation get memory space that is
  // deallocated by free operation at current epoch
  if(get_phase(epoch) ==
     epoch_phase::Multi_Epoch_Existence) { // must delay
    DEBUG_PRINT("free operation delays");
    _delay_dealloc_list[curr].push_back(addr);
#ifdef ESCORT_PROF
    _delay_dealloc_list[curr]++;
#endif /* ESCORT_PROF */
  } else {
    _escort_internal_free(addr);
  }
	      
  if(is_out_of_transaction) {
    end_op();
  }
#else /* SAVE_ALLOCATOR */
  epoch_t epoch = get_epoch(std::memory_order_relaxed);
  if(epoch == 0) {
    begin_op();
    free_with_create_log(addr, size);
    end_op();
  } else {
    bool curr = static_cast<bool>(epoch & 0x1);
    if(get_phase(epoch) == epoch_phase::Multi_Epoch_Existence) {
      DEBUG_PRINT("free operation delays");
      _delay_dealloc_list[curr].push_back(addr);
#ifdef ESCORT_PROF
      _delay_dealloc_list_size[curr]++;
#endif /* ESCORT_PROF */
    } else {
      pm_free(addr);
    }
  }
#endif /* SAVE_ALLOCATOR */
}

void* userthreadctx_t::malloc(std::size_t size){
#ifdef NO_PERSIST_METADATA
  return pm_malloc(size);
#else
  return malloc_with_create_log(size);
#endif
}

void userthreadctx_t::free(void* addr, std::size_t size) {
#ifdef NO_PERSIST_METADATA
  pm_free(addr);
#else
  free_with_create_log(addr, size);
#endif
}

// After GLOBAL_EPOCH is assigned into the register,
// GLOBAL_EOPCH is updated, and in addition,
// the wait for the end of the transactions at the previous epoch
// may be over.
// Simple assignment would assign the previous GLOBAL_EPOCH
// during checkpointing. So, check epoch after assignment.
void userthreadctx_t::begin_op() {
#ifdef WEAK_CONSISTENCY
  set_epoch(GLOBAL_EPOCH, std::memory_order_relaxed);
#else
  do {
    set_epoch(get_global_epoch(), std::memory_order_seq_cst);
  } while(get_epoch(std::memory_order_relaxed) != get_global_epoch());
  if(get_epoch(std::memory_order_relaxed) == 0)
    std::cout << get_global_epoch() << std::endl;
  assert(get_epoch(std::memory_order_relaxed) != 0);
#endif
}

void userthreadctx_t::end_op() {
  assert(get_epoch(std::memory_order_relaxed) != 0);
  set_epoch(0, std::memory_order_relaxed);
}

void userthreadctx_t::mark(void* addr, std::size_t size) {
  epoch_t epoch = get_epoch(std::memory_order_relaxed);
  assert(epoch != 0);
  
  bool curr = static_cast<bool>(epoch & 0x1);
  bool prev = (!curr);

  std::size_t index = gv::BitMap[curr]->get_index(addr);
  std::size_t num_bits = (size + CACHE_LINE_SIZE - 1) >> LOG_CACHE_LINE_SIZE;
  std::size_t end_index = num_bits + index;
  
  bool is_append_list = false;
  auto addr_ptr = reinterpret_cast<std::intptr_t>(addr);
  addr_ptr -= addr_ptr%CACHE_LINE_SIZE;
  addr = reinterpret_cast<void*>(addr_ptr);

#ifndef OLD_VERSION
  std::pair<void*, std::size_t> addrlist_entry = {addr, num_bits};
  for(; index < end_index; index++) {
    if(!gv::BitMap[curr]->is_set(index)) { // set bit
      if(gv::BitMap[prev]->is_set(index)) { // check whether CoW
	debug::add_plog_user();
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
#else
  for(; index < end_index; index++) {
    if(!gv::BitMap[curr]->is_set(index)) { // set bit
      if(gv::BitMap[prev]->is_set(index)) { // check whether CoW
	debug::add_plog_user();
	cacheline_t &old_val = *(reinterpret_cast<cacheline_t*>(addr));
	gv::BitMap[prev]->clear(index);
	_log->push_back_and_clwb({reinterpret_cast<cacheline_t*>(addr), old_val});
      }
      gv::BitMap[curr]->set(index);
      _addrlist[curr]->append(addr);
    }
    addr = add_addr(addr, CACHE_LINE_SIZE);
  }
#endif  
}

void userthreadctx_t::set_root(const char* id, void* addr) {
#ifdef ALLOCATOR_RALLOC
#warning set_root is not implemented
#else // ALLOCATOR_RALLOC
  assert(get_epoch(std::memory_order_relaxed) != 0);
  gv::RootTable->set(id, addr);
#endif // ALLOCATOR_RALLOC
}
void* userthreadctx_t::get_root(const char* id) {
#ifdef ALLOCATOR_RALLOC
#warning get_root is not implemented
  return nullptr;
#else // ALLOCATOR_RALLOC
  assert(get_epoch(std::memory_order_relaxed) != 0);
  return gv::RootTable->get(id);
#endif // ALLOCATOR_RALLOC
}
void userthreadctx_t::remove_root(const char* id) {
#ifdef ALLOCATOR_RALLOC
#warning remove_root is not implemented
#else // ALLOCATOR_RALLOC
  assert(get_epoch(std::memory_order_relaxed) != 0);
  return gv::RootTable->remove(id);
#endif // ALLOCATOR_RALLOC
}
