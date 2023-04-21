#include <cstring>

#include "../config.h"

#include "recovery.hpp"
#include "globalEscort.hpp"
#include "plog_t.hpp"
#include "allocatorlog_t.hpp"
#include "bytemap_t.hpp"
#include "nvm/config.hpp"

namespace gv = Escort::GlobalVariable;

#ifndef OLD_VERSION
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
  DEBUG_PRINT("copy");
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
}

#ifdef SAVE_ALLOCATOR
void Escort::recovery::internal::replay_allocator_logs() {
  auto all_allocatorlog_list =
    gv::Allocatorlog_Management->free_list();
  for(auto block: all_allocatorlog_list) {
    std::size_t size = block->size();
    epoch_t epoch = block->epoch();
    if(Epoch(epoch) == Epoch(GLOBAL_EPOCH)-1) {}
      
    const auto entries = block->entries();
    _mm_sfence();
    block->clear();
  }
  _mm_sfence();
}

void Escort::recovery::apply_metadata() {
  DEBUG_PRINT("apply_metadata");
  if(Escort::get_phase(GLOBAL_EPOCH)
     == Escort::epoch_phase::Log_Persistence)
    Escort::recovery::internal::replay_allocator_logs();
  std::size_t bytemap_size = Escort::HEAP_SIZE * sizeof(byte) / CACHE_LINE_SIZE;

  for(int i = 0; i < bytemap_size; i++) {
    auto bytei = gv::ByteMap_NVM->get_byte(i);
    if(bytei >= 128) {
      i += (bytei/CACHE_LINE_SIZE) - 1;
    }
    void* addr = add_addr(gv::NVM_config->dram_space(), i * CACHE_LINE_SIZE);
    std::size_t size = bytei * CACHE_LINE_SIZE;
    // void* ret = escort_malloc_with_addr(addr, size);
    void* ret = _escort_internal_malloc(size);
  }
}
#endif /* SAVE_ALLOCATOR */
#else // OLD_VERSION does not support recovery functions
void Escort::recovery::replay_redo_logs() {}
void Escort::recovery::copy() {}
void Escort::recovery::internal::replay_allocator_logs(){}
void Escort::recovery::apply_metadata() {}
#endif
