#include <cstring>

#include "recovery.hpp"
#include "globalEscort.hpp"
#include "plog_t.hpp"
#include "allocatorlog_t.hpp"
#include "nvm/config.hpp"

namespace gv = Escort::GlobalVariable;

void Escort::recovery::replay_redo_logs() {
  DEBUG_PRINT("replay_redo_logs");
  auto all_plog_list =  gv::Plog_Management->free_list();
  for(auto block: all_plog_list) {
    std::size_t size = block->size();
    const auto entries = block->entries();
    for(int i = 0; i < size; i++) {
      const auto& entry = entries[i];
      cacheline_t* nvm_cl = add_addr(entry.addr, gv::NVM_config->offset());
      *nvm_cl = entry.val;
      _mm_clwb(nvm_cl);
    }
    _mm_sfence();
    block->clear();
  }
  _mm_sfence();
}

void Escort::recovery::copy() {
  auto dram_space = reinterpret_cast<char*>(gv::NVM_config->dram_space());
  auto nvm_space = reinterpret_cast<char*>(gv::NVM_config->nvm_space());
  DEBUG_PRINT("dram_space", (void*) dram_space, "nvm_space", (void*) nvm_space);
  const std::size_t block_size = 4096;
  for(std::size_t i = 0; i < HEAP_SIZE; i += block_size) {
    int is_protect = mprotect(dram_space, block_size, PROT_READ|PROT_WRITE);
    if(is_protect == -1) {
      dram_space += block_size;
      nvm_space += block_size;
      continue;
    }
    for(std::size_t j = 0; j < block_size; j++) {
      if(*dram_space == 0 && *nvm_space != 0)
	*dram_space = *nvm_space;
      dram_space++;
      nvm_space++;
    }
  }
  
  DEBUG_PRINT("copy_end");
}

void Escort::recovery::apply_metadata() {
}
