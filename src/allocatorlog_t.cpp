#include "allocatorlog_t.hpp"

namespace gv = Escort::GlobalVariable;

void Escort::allocatorlog_t::append(std::pair<void*, std::size_t> entry, epoch_t epoch) {
  if(_block_list.empty()) { // first exec in each epoch
    log_block_t* tail_init = gv::Allocatorlog_Management->pop();
    tail_init->set_epoch(epoch);
    _block_list.push_back(tail_init);
  }
  log_block_t* tail = _block_list.back();
  bool is_succ = tail->append(entry); // append
    
  if(!is_succ) { // get new block and append to new block
    log_block_t* tail_new = gv::Allocatorlog_Management->pop();
    tail_new->set_epoch(epoch);
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
