#include <cassert>

#include "Escort.hpp"
#include "globalEscort.hpp"
#include "userthreadctx_t.hpp"
#include "checkpointing.hpp"
#include "debug.hpp"
#include "init.hpp"
#include "recovery.hpp"
#include "nvm/config.hpp"

namespace gv = Escort::GlobalVariable;
namespace lv = Escort::LocalVariable;

void Escort_init(const char* nvm_path, std::uint32_t checkpointing_num, bool is_recovery) {
  DEBUG_PRINT("Escort_init");
  if(gv::NVM_config == nullptr)
    gv::NVM_config = NEW(Escort::nvmconfig_t, nvm_path);

  Escort::init::init_bitmap();
  Escort::init::init_manager();
  
  if(is_recovery) {
    Escort::epoch_t epoch = Escort::Epoch(GLOBAL_EPOCH);
    DEBUG_PRINT("NVM is dirty", epoch);
    // recovery
    if(Escort::get_phase(epoch)
       == Escort::epoch_phase::Log_Persistence) 
      Escort::recovery::replay_redo_logs();
    
    Escort::recovery::copy();
    
    Escort::recovery::apply_metadata();
    
    gv::RootTable = gv::NVM_config->get_roottable();
    DEBUG_PRINT("roottable:", gv::RootTable);
    gv::NVM_config->reset_thread_num();
  } else {
    DEBUG_PRINT("First execution");
    GLOBAL_EPOCH = 2;
    Escort::init::init_roottable();
  }

  Escort::init::init_checkpointing_thread(checkpointing_num);

  gv::NVM_config->flush_variable();
}

bool Escort_is_recovery(const char* nvm_path) {
  if(gv::NVM_config == nullptr)
    gv::NVM_config = NEW(Escort::nvmconfig_t, nvm_path);
  return gv::NVM_config->is_dirty();
}

void Escort_finalize(){
  DEBUG_PRINT("Escort_finalize");
  gv::isPersisting = false;
  gv::CpMaster->join();
  DELETE(gv::NVM_config);
}

void Escort_thread_init() {
  assert(lv::ctx == nullptr);
  lv::ctx = NEW(Escort::userthreadctx_t);
  gv::CpMaster->init_userthreadctx(lv::ctx);
}

void Escort_thread_finalize() {
  // under construction
  gv::CpMaster->finalize_userthreadctx(lv::ctx);
}
 
void* Escort_malloc(std::size_t size) {
  assert(lv::ctx != nullptr);
  return lv::ctx->malloc(size);
}
void Escort_free(void* addr, std::size_t size) {
  assert(lv::ctx != nullptr);
  lv::ctx->free(addr, size);
}
void Escort_begin_op() {
  assert(lv::ctx != nullptr);
  lv::ctx->begin_op();
}
void Escort_end_op() {
  assert(lv::ctx != nullptr);
  lv::ctx->end_op();
}
void Escort_write_region(void* addr, std::size_t size) {
  assert(lv::ctx != nullptr);
  lv::ctx->mark(addr, size);
}
void Escort_set_root(const char* id, void* addr) {
  assert(lv::ctx != nullptr);
  lv::ctx->set_root(id, addr);
}
void* Escort_get_root(const char* id) {
  assert(lv::ctx != nullptr);
  return lv::ctx->get_root(id);
}
void Escort_remove_root(const char* id) {
  assert(lv::ctx != nullptr);
  lv::ctx->get_root(id);
}
