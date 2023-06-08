#ifdef ALLOCATOR_RALLOC

#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>
#include <string>
#include "metadata.hpp"
#include "allocator.hpp"
#include "debug.hpp"

// escort1 header
#include "bitmap_t.hpp"
#include "globalEscort.hpp"
#include "recovery.hpp"
#include "plog_t.hpp"
#include "checkpointing.hpp"

using namespace Escort;
using namespace std;

namespace Escort {
  static PersistentMetaData* init_heap(const char* nvm_path, uint64_t heap_size, bool recovery);
}

// escort1 namespace
namespace gv = Escort::GlobalVariable;
namespace lv = Escort::LocalVariable;

static PersistentMetaData* Escort::init_heap(const char* nvm_path, uint64_t heap_size, bool recovery)
{
  string filename = string(nvm_path) + "_escort";
  PersistentMetaData* md = new PersistentMetaData();
  if (md->open(filename.c_str()) != 0)
    return nullptr;
  if (recovery) {
    if (!md->is_valid()) {
      fprintf(stderr, "cannot recovery from broken file\n");
      return nullptr;
    }
    pm_init(nvm_path, heap_size, md->get_dram_base());
  } else {
    pm_init(nvm_path, heap_size, nullptr);
    md->init(dram_base);
  }
  return md;
}

// Escort API
void escort_init(const char* nvm_path, uint32_t num_checkpointing_threads, bool recovery)
{
  DEBUG_PRINT(__func__);
#if 0
  // Determine the file path.
  // In escort.cpp, "_sb" is added as a file suffix.
  string sb_file = string(nvm_path) + "_sb";
  struct stat64 st;
  if (stat64(sb_file.c_str(), &st) != 0) {
    perror("stat64");
    exit(1);
  }
#else
#warning TODO: heap size
#endif
  PersistentMetaData* md = Escort::init_heap(nvm_path, 8L * 1024 * 1024 * 1024, recovery);
  persistent_metadata = md;

  // initialize bitmap
  for(int i = 0; i < 2; i++)
    gv::BitMap[i] = new bitmap_t(sb_region_size);

  // initialize manager
  gv::Plog_Management = new plog_management_t(md->get_plog_area_addr(), md->get_plog_area_size());

  if(recovery) {
    epoch_t epoch = md->get_epoch();
    if (get_phase(epoch) == epoch_phase::Log_Persistence)
      recovery::replay_redo_logs();
    recovery::copy();
  } else {
    md->set_epoch(2);
  }

  // start checkpointing thread
  gv::CpMaster = new cpmaster_t(num_checkpointing_threads);
  gv::CpMaster->run();
}

// Escort API
bool escort_is_recovery(const char* nvm_path) {
  DEBUG_PRINT(__func__);
  string filename = string(nvm_path) + "_escort";
  PersistentMetaData md;
  if (md.open(filename.c_str()) != 0)
    return false;
  return md.is_valid();
}

// Escort API
void escort_finalize(){
  DEBUG_PRINT(__func__);
  gv::isPersisting = false;
  gv::CpMaster->join();
  delete persistent_metadata;
  persistent_metadata = nullptr;
}

// Escort API
void escort_thread_init() {
  DEBUG_PRINT(__func__);
  assert(lv::ctx == nullptr);
  lv::ctx = new Escort::userthreadctx_t();
  gv::CpMaster->init_userthreadctx(lv::ctx);
}

void escort_thread_finalize() {
  // under construction
  gv::CpMaster->finalize_userthreadctx(lv::ctx);
}
 
void* escort_malloc(std::size_t size) {
  assert(lv::ctx != nullptr);
  return lv::ctx->malloc(size);
}
void escort_free(void* addr, std::size_t size) {
  assert(lv::ctx != nullptr);
  lv::ctx->free(addr, size);
}
void escort_begin_op() {
  assert(lv::ctx != nullptr);
  lv::ctx->begin_op();
}
void escort_end_op() {
  assert(lv::ctx != nullptr);
  lv::ctx->end_op();
}
void escort_write_region(void* addr, std::size_t size) {
  assert(lv::ctx != nullptr);
  lv::ctx->mark(addr, size);
}
void escort_set_root(const char* id, void* addr) {
  assert(lv::ctx != nullptr);
  lv::ctx->set_root(id, addr);
}
void* escort_get_root(const char* id) {
  assert(lv::ctx != nullptr);
  return lv::ctx->get_root(id);
}
void escort_remove_root(const char* id) {
  assert(lv::ctx != nullptr);
  lv::ctx->get_root(id);
}

void escort_get_current_status(int* epoch, int* phase,
			       int* num_user_threads, int* num_used_alloc_logs,
			       int* num_delayed_deallocs)
{
  PersistentMetaData* md = persistent_metadata;
  epoch_t global_epoch = md->get_epoch();
  *epoch = (int) Escort::Epoch(global_epoch);
  *phase = (int) Escort::get_phase(global_epoch);
  *num_user_threads = Escort::GlobalVariable::CpMaster->get_num_user_threads();
#ifdef SAVE_ALLOCATOR
  *num_used_alloc_logs =
    Escort::GlobalVariable::Allocatorlog_Management->get_num_used_alloc_logs();
#else /* SAVE_ALLOCATOR */
  *num_used_alloc_logs = 0;
#endif /* SAVE_ALLOCATOR */
#ifdef ESCORT_PROF
  *num_delayed_deallocs =
    Escort::GlobalVariable::CpMaster->get_num_delayed_deallocs();
#else /* ESCORT_PROF */
  *num_delayed_deallocs = 0;
#endif /* ESCORT_PROF */
}


#else // ALLOCATOR_RALLOC



#include <cassert>

#include "../config.h"

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

using namespace Escort;
using namespace std;

void escort_init(const char* nvm_path, std::uint32_t checkpointing_num, bool is_recovery) {
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

#ifdef SAVE_ALLOCATOR
    Escort::recovery::apply_metadata();
#endif /* SAVE_ALLOCATOR */
    
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

bool escort_is_recovery(const char* nvm_path) {
  if(gv::NVM_config == nullptr)
    gv::NVM_config = NEW(Escort::nvmconfig_t, nvm_path);
  return gv::NVM_config->is_dirty();
}

void escort_finalize(){
  DEBUG_PRINT("Escort_finalize");
  gv::isPersisting = false;
  gv::CpMaster->join();
  DELETE(gv::NVM_config);
}

void escort_thread_init() {
  assert(lv::ctx == nullptr);
  lv::ctx = NEW(Escort::userthreadctx_t);
  gv::CpMaster->init_userthreadctx(lv::ctx);
}

void escort_thread_finalize() {
  // under construction
  gv::CpMaster->finalize_userthreadctx(lv::ctx);
}
 
void* escort_malloc(std::size_t size) {
  assert(lv::ctx != nullptr);
  return lv::ctx->malloc(size);
}
void escort_free(void* addr, std::size_t size) {
  assert(lv::ctx != nullptr);
  lv::ctx->free(addr, size);
}
void escort_begin_op() {
  assert(lv::ctx != nullptr);
  lv::ctx->begin_op();
}
void escort_end_op() {
  assert(lv::ctx != nullptr);
  lv::ctx->end_op();
}
void escort_write_region(void* addr, std::size_t size) {
  assert(lv::ctx != nullptr);
  lv::ctx->mark(addr, size);
}
void escort_set_root(const char* id, void* addr) {
  assert(lv::ctx != nullptr);
  lv::ctx->set_root(id, addr);
}
void* escort_get_root(const char* id) {
  assert(lv::ctx != nullptr);
  return lv::ctx->get_root(id);
}
void escort_remove_root(const char* id) {
  assert(lv::ctx != nullptr);
  lv::ctx->get_root(id);
}

void escort_get_current_status(int* epoch, int* phase,
			       int* num_user_threads, int* num_used_alloc_logs,
			       int* num_delayed_deallocs)
{
  *epoch = (int) Escort::Epoch(GLOBAL_EPOCH);
  *phase = (int) Escort::get_phase(GLOBAL_EPOCH);
  *num_user_threads = Escort::GlobalVariable::CpMaster->get_num_user_threads();
#ifdef SAVE_ALLOCATOR
  *num_used_alloc_logs =
    Escort::GlobalVariable::Allocatorlog_Management->get_num_used_alloc_logs();
#else /* SAVE_ALLOCATOR */
  *num_used_alloc_logs = 0;
#endif /* SAVE_ALLOCATOR */
#ifdef ESCORT_PROF
  *num_delayed_deallocs =
    Escort::GlobalVariable::CpMaster->get_num_delayed_deallocs();
#else /* ESCORT_PROF */
  *num_delayed_deallocs = 0;
#endif /* ESCORT_PROF */
}

#endif // ALLOCATOR_RALLOC