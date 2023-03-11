#include "plog_t.hpp"

namespace gv = Escort::GlobalVariable;

void Escort::plog_t::push_back_and_clwb(std::pair<cacheline_t*, cacheline_t&> entry) {
  if(_block_list.empty()) { // first exec in each epoch
    log_block_t* tail_init = gv::Plog_Management->pop();
    _block_list.push_back(tail_init);
  }
  log_block_t* tail = _block_list.back();
  bool is_succ = tail->append(entry); // append
  if(!is_succ) {
    log_block_t* tail_new = gv::Plog_Management->pop();
    _block_list.push_back(tail_new);
    tail_new->append(entry);
  }
}

void Escort::plog_t::flush() {
  for(log_block_t* block: _block_list)
    block->flush();
}

void Escort::plog_t::clear() {
  while(!_block_list.empty()) {
    log_block_t* tail = _block_list.back();
    tail->clear();
    gv::Plog_Management->push(tail);
    _block_list.pop_back();
  }
  _mm_sfence();
}
