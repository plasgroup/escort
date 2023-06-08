#include "../config.h"

#include "metadata.hpp"

#include "globalEscort.hpp"
#include "Escort.hpp"
#include "debug.hpp"

namespace Escort {
  // std::uint32_t LOG_CACHE_LINE_SIZE = 6;
  // std::uint32_t CACHE_LINE_SIZE = (1 << LOG_CACHE_LINE_SIZE);
  std::size_t HEAP_SIZE = calc_heapsize(8);
#ifndef ALLOCATOR_RALLOC
  std::intptr_t DRAM_BASE = 0x7f9f14abd000;
#endif // ALLOCATOR_RALLOC

#ifdef ALLOCATOR_RALLOC
  PersistentMetaData* persistent_metadata = nullptr;
#endif // ALLOCATOR_RALLOC
  namespace GlobalVariable {
#ifndef ALLOCATOR_RALLOC
    nvmconfig_t *NVM_config = nullptr;
    epoch_t *Epoch = nullptr; // place on NVM
#endif // ALLOCATOR_RALLOC
    bitmap_t *BitMap[2];
#ifndef ALLOCATOR_RALLOC
    bytemap_dram_t *ByteMap[2];
    bytemap_nvm_t *ByteMap_NVM;
    roottable_t *RootTable;
#endif // ALLOCATOR_RALLOC
    bool isPersisting = true; // set false to signal checkpointing thread to terminate.
    std::uint32_t EpochLength = 100;
    cpmaster_t *CpMaster = nullptr;
    plog_management_t *Plog_Management = nullptr;
#ifdef SAVE_ALLOCATOR
    allocatorlog_management_t *Allocatorlog_Management = nullptr;
#endif /* SAVE_ALLOCATOR */
  }
  
  namespace LocalVariable {
    thread_local userthreadctx_t *ctx = nullptr;
  }
  
  // higher 2-bit means phase
  const epoch_t epoch_mask                    = 0x3fffffffffffffff;
  const epoch_t mask_multiple_epoch_existence = 0x8000000000000000;
  const epoch_t mask_log_persistence          = 0x4000000000000000;
  const epoch_t mask_nvm_heap_update          = 0x0000000000000000;

  epoch_t Epoch(epoch_t epoch) { return epoch & epoch_mask; }
  epoch_t set_phase(epoch_t epoch, epoch_phase phase) {
    epoch &= epoch_mask;
    switch(phase) {
    case epoch_phase::Multi_Epoch_Existence:
      epoch |= mask_multiple_epoch_existence;
      break;
    case epoch_phase::Log_Persistence:
      epoch |= mask_log_persistence;
      break;
    case epoch_phase::NVM_Heap_Update:
      epoch |= mask_nvm_heap_update;
      break;
    default:
      DEBUG_ERROR("not reacheable, epoch: ", epoch);
      break;
    }
    return epoch;
  }

  epoch_phase get_phase(epoch_t epoch) {
    epoch_t mask_phase = (epoch&epoch_mask)^epoch;
    switch(mask_phase) {
    case mask_multiple_epoch_existence:
      return epoch_phase::Multi_Epoch_Existence;
    case mask_log_persistence:
      return epoch_phase::Log_Persistence;
    case mask_nvm_heap_update:
      return epoch_phase::NVM_Heap_Update;
    default:
      DEBUG_ERROR("not reachable");
      break;
    }
    return epoch_phase::Default;
  }
}
