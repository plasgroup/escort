#include "../config.h"

#include "init.hpp"
#include "roottable_t.hpp"
#include "bitmap_t.hpp"
#include "bytemap_t.hpp"
#include "plog_t.hpp"
#include "allocatorlog_t.hpp"
#include "checkpointing.hpp"
#include "debug.hpp"

namespace gv = Escort::GlobalVariable;

#ifndef ALLOCATOR_RALLOC
void Escort::init::init_roottable() {
  gv::RootTable = reinterpret_cast<roottable_t*>(_escort_internal_malloc(sizeof(roottable_t)));
  new(gv::RootTable) roottable_t(); // must exist on DRAM heap
  DEBUG_PRINT("gv::RootTable:", gv::RootTable);
  gv::NVM_config->set_root_pointer(gv::RootTable);
}

void Escort::init::init_bitmap() {
  for(int i = 0; i < 2; i++) {
    assert(gv::BitMap[i] == nullptr);
    gv::BitMap[i] = NEW(bitmap_t, Escort::HEAP_SIZE);
    assert(gv::ByteMap[i] == nullptr);
    gv::ByteMap[i] = NEW(bytemap_dram_t, Escort::HEAP_SIZE);
  }
  assert(gv::ByteMap_NVM == nullptr);
  gv::ByteMap_NVM = gv::NVM_config->pnew<bytemap_nvm_t>(Escort::HEAP_SIZE);
  gv::NVM_config->set_bytemap_pointer(gv::ByteMap_NVM);
}

void Escort::init::init_manager() {
  gv::Plog_Management = NEW(plog_management_t);
#ifdef SAVE_ALLOCATOR
  gv::Allocatorlog_Management
    = NEW(allocatorlog_management_t, 8192 * 4);
#endif /* SAVE_ALLOCATOR */
}

void Escort::init::init_checkpointing_thread(std::uint32_t checkpointing_num) {
  assert(gv::CpMaster == nullptr);
  gv::NVM_config->set_checkpointing_thread(checkpointing_num);
  gv::CpMaster = NEW(cpmaster_t, checkpointing_num);
  gv::CpMaster->run();
}
#endif // ALLOCATOR_RALLOC

