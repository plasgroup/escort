#include <chrono>
#include <thread>
#include <utility>

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <x86intrin.h>

#include "checkpointing.hpp"
#include "userthreadctx_t.hpp"
#include "bitmap_t.hpp"
#include "debug.hpp"

namespace gv = Escort::GlobalVariable;

void Escort::cpmaster_t::wait_previous_epoch_transactions(epoch_t epoch) {
  // wait for finishing previous epoch's transactions
  for(userthreadctx_t *ctx: _ctx_array) {
    epoch_t epoch_ctx = ctx->get_epoch();
    while(Epoch(epoch_ctx) == (Epoch(epoch)-1)) {
      epoch_ctx = ctx->get_epoch();
    }
  }
}

void Escort::cpmaster_t::log_persistence_unit() {
  auto prev = static_cast<bool>((Epoch(GLOBAL_EPOCH)-1) & 0x1);
  for(auto ctx: _ctx_array) {
    DEBUG_PRINT("list_size:", ctx->list(prev).size());

    for(const auto&[addr, size]: ctx->list(prev)) {
      cacheline_t* cl = reinterpret_cast<cacheline_t*>(addr);
      size_t base = gv::BitMap[prev]->get_index(cl);
      for(int i = 0; i < size; i++) {
    	cacheline_t& value = *cl;
	_log->push_back_and_clwb({cl, value});
    	gv::BitMap[prev]->clear(base+i);
    	cl++;
      }
    }
    ctx->list(prev).clear();
  }
}

void Escort::cpmaster_t::log_persistence_multi() {
  // under construction
  DEBUG_ERROR("under construction");
}

void Escort::cpmaster_t::log_persistence() {
  run_check_valid_cpallocator();
  
  if(is_unit)
    log_persistence_unit();
  else
    log_persistence_multi();
  
  join_cpallocator();
}

void Escort::cpmaster_t::replay_redo_log(plog_t& log) {
  auto block_list = log.block_list();
  for(plog_t::log_block_t* block: block_list) {
    std::size_t size = block->size();
    const auto entries = block->entries();
    for(int i = 0; i < size; i++) {
      const auto& entry = entries[i];
      cacheline_t* nvm_cl = add_addr(entry.addr, gv::NVM_config->offset());
      *nvm_cl = entry.val;
      _mm_clwb(nvm_cl);
    }
  }
}

void Escort::cpmaster_t::clear_redo_logs() {
  _log->clear();
  for(userthreadctx_t *ctx: _ctx_array)
    ctx->log().clear();
  
  _mm_sfence();
}

void Escort::cpmaster_t::nvm_heap_update_unit() {
  replay_redo_log(*_log);

  _mm_sfence();
  
  for(userthreadctx_t *ctx: _ctx_array) 
    replay_redo_log(ctx->log());
  
  _mm_sfence();
  
  clear_redo_logs();
}

void Escort::cpmaster_t::nvm_heap_update_multi() {
  // under construction
  DEBUG_ERROR("under construction");
}
void Escort::cpmaster_t::nvm_heap_update() {
  run_checkpointing_cpallocator();
  
  if(is_unit)
    nvm_heap_update_unit();
  else
    nvm_heap_update_multi();

  join_cpallocator();
}

// cpu_bind() suppose NVM is
// connected with first socket
// if NVM is connected with second socket,
// change obj->children[0] => obj->children[1]  
void Escort::cpmaster_t::cpu_bind() {
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  hwloc_obj_t obj = hwloc_get_root_obj(topology);
  while(obj->type < HWLOC_OBJ_SOCKET) {
    obj = obj->children[0]; // use CPUs connected with NVM
  }
  affinity = obj;
  if(affinity != nullptr) {
    hwloc_set_cpubind(topology, affinity->cpuset, HWLOC_CPUBIND_THREAD);
  }
}

void Escort::cpmaster_t::finalize(epoch_t epoch) {
  _mm_sfence();
  epoch = set_phase(epoch+1, epoch_phase::Log_Persistence);
  GLOBAL_EPOCH = epoch;
  _mm_mfence();

  log_persistence();

  _mm_sfence();
  epoch = set_phase(epoch, epoch_phase::NVM_Heap_Update);
  GLOBAL_EPOCH = epoch;
  _mm_mfence();

  nvm_heap_update();
}

void Escort::cpmaster_t::checkpointing() {
  cpu_bind(); // bind cpu connected NVM directly
  epoch_t epoch = Epoch(GLOBAL_EPOCH);
  
  while(gv::isPersisting) {
    DEBUG_PRINT("checkpionting", epoch);
    while(!has_execute_right()) { /* wait */ }
    auto time_start = std::chrono::system_clock::now();
    
    _mm_sfence();
    epoch = set_phase(epoch+1, epoch_phase::Multi_Epoch_Existence);
    GLOBAL_EPOCH = epoch;
    _mm_mfence();

    wait_previous_epoch_transactions(epoch);
    
    epoch = set_phase(epoch, epoch_phase::Log_Persistence);
    GLOBAL_EPOCH = epoch;

    run_deallocator(); // for deallocation of  delayed free operations
    
    log_persistence();

    _mm_sfence();
    epoch = set_phase(epoch, epoch_phase::NVM_Heap_Update);
    GLOBAL_EPOCH = epoch;
    _mm_mfence();

    nvm_heap_update();

    join_deallocator(); // join deallocator 
    
    release_execute_right();
    
    auto time_end   = std::chrono::system_clock::now();
    auto checkpointing_time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_end).count());
    if(checkpointing_time < gv::EpochLength)
      std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<std::uint64_t>(gv::EpochLength - checkpointing_time)));

    while(_num_wait_threads.load(std::memory_order_acquire) > 0) { /* wait */ }
  }

  finalize(epoch);
}

void Escort::cpmaster_t::init_userthreadctx(userthreadctx_t* ctx) {
  _num_wait_threads.fetch_add(1);

  while(!has_execute_right()) { /* wait */ }
  
  DEBUG_PRINT("add user");
  
  _ctx_array.push_back(ctx);
  gv::NVM_config->add_user_num();

  _num_wait_threads.fetch_sub(1);
  release_execute_right();
}

void Escort::cpmaster_t::finalize_userthreadctx(userthreadctx_t* ctx) {
  _num_wait_threads.fetch_add(1);
  while(!has_execute_right()) { /* wait */ }
  
  DEBUG_PRINT("sub user");
  _ctx_delete_array.push_back(ctx);

  _num_wait_threads.fetch_sub(1);
  release_execute_right();
}
