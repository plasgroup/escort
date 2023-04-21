#ifdef SAVE_ALLOCATOR

#include "../config.h"

#include "allocatorlog_t.hpp"

namespace gv = Escort::GlobalVariable;

void Escort::allocatorlog_t::append(std::pair<void*, std::size_t> entry, epoch_t epoch) {
  if(_block_list.empty()) { // first exec in each epoch
    log_block_t* tail_init = gv::Allocatorlog_Management->pop();
#ifdef DEBUG
    assert(tail_init->_owner_alloc == NULL);
    printf("alloclog assign %p -> %p ep = %lx (GEP=%lx)\n", tail_init, this, epoch, GLOBAL_EPOCH);
#endif /* DEBUG */
    tail_init->set_epoch(epoch);
#ifdef DEBUG
    tail_init->set_owner(this, std::this_thread::get_id());
    printf("%p _block_list.push_back(%p) epoch = %lx\n", this, tail_init, epoch);
#endif /* DEBUG */
    _block_list.push_back(tail_init);
  }
  log_block_t* tail = _block_list.back();
#ifdef DEBUG
  tail->verify_owner(this, std::this_thread::get_id());
#endif /* DEBUG */
  bool is_succ = tail->append(entry); // append
    
  if(!is_succ) { // get new block and append to new block
    log_block_t* tail_new = gv::Allocatorlog_Management->pop();
#ifdef DEBUG
    assert(tail_new->_owner_alloc == NULL);
    printf("alloclog assign %p -> %p ep = %lx (GEP=%lx)\n", tail_new, this, epoch, GLOBAL_EPOCH);
#endif /* DEBUG */
    tail_new->set_epoch(epoch);
#ifdef DEBUG
    tail_new->set_owner(this, std::this_thread::get_id());
    printf("%p _block_list.push_back(%p) epoch = %lx (2)\n", this, tail_new, epoch);
#endif /* DEBUG */
    _block_list.push_back(tail_new);
    tail_new->append(entry);
  }
}

void Escort::allocatorlog_t::clear() {
  while(!_block_list.empty()) {
    log_block_t* tail = _block_list.back();
    tail->clear();
    gv::Allocatorlog_Management->push(tail);
    _block_list.pop_back();
  }
  _mm_sfence();
}

#endif /* SAVE_ALLOCATOR */
