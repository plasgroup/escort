#include <chrono>
#include <thread>
#include <utility>

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <x86intrin.h>

#include "allocator.hpp"
#include "metadata.hpp"

#include "../config.h"

#include "checkpointing.hpp"
#include "userthreadctx_t.hpp"
#include "bitmap_t.hpp"
#include "debug.hpp"

namespace gv = Escort::GlobalVariable;

using namespace std;

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
#ifndef OLD_VERSION
  auto prev = static_cast<bool>((Epoch(get_global_epoch())-1) & 0x1);
  for(auto ctx: _ctx_array) {
    DEBUG_PRINT("list_size:", ctx->list(prev).size());
    for(const auto&[addr, size]: ctx->list(prev)) {
      cacheline_t* cl = reinterpret_cast<cacheline_t*>(addr);
      std::size_t base = gv::BitMap[prev]->get_index(cl);
      debug::add_plog(size);
      
      for(int i = 0; i < size; i++) {
    	cacheline_t& value = *cl;
	if(gv::BitMap[prev]->is_set(base+i)) {
	  _log->push_back_and_clwb({cl, value});
	  gv::BitMap[prev]->clear(base+i);
	} // if bit is clear, CoW has happened
    	cl++;
      }
    }
    ctx->list(prev).clear();
  }
#else
  auto prev = static_cast<bool>((Epoch(GLOBAL_EPOCH)-1) & 0x1);
  for(auto ctx: _ctx_array) {
    auto addrlist = ctx->list(prev);
    debug::add_plog(addrlist->size());
    while(!addrlist->is_empty()) {
      cacheline_t* cl = reinterpret_cast<cacheline_t*>(addrlist->pop());
      cacheline_t& value = *cl;
      std::size_t idx = gv::BitMap[prev]->get_index(cl);
      if(gv::BitMap[prev]->is_set(idx)) {
	_log->push_back_and_clwb({cl, value});
	gv::BitMap[prev]->clear(idx);
      } // if bit is clear, CoW has happened
    }
  }
#endif  
}

void Escort::cpmaster_t::log_persistence_multi() {
  // under construction
  DEBUG_ERROR("under construction");
}

void Escort::cpmaster_t::log_persistence() {
#ifdef SAVE_ALLOCATOR
  run_check_valid_cpallocator();
#endif /* SAVE_ALLOCATOR */
  
  if(is_unit)
    log_persistence_unit();
  else
    log_persistence_multi();
  
#ifdef SAVE_ALLOCATOR
  join_cpallocator();
#endif /* SAVE_ALLOCATOR */
}

void Escort::cpmaster_t::replay_redo_log(plog_t& log) {
#ifndef OLD_VERSION
  auto block_list = log.block_list();
  for(plog_t::log_block_t* block: block_list) {
    std::size_t size = block->size();
    const auto entries = block->entries();
    for(int i = 0; i < size; i++) {
      const auto& entry = entries[i];
#ifdef ALLOCATOR_RALLOC
      cacheline_t* nvm_cl = add_addr(entry.addr, -nvm_dram_distance);
#else // ALLOCATOR_RALLOC
      cacheline_t* nvm_cl = add_addr(entry.addr, gv::NVM_config->offset());
#endif // ALLOCATOR_RALLOC
      *nvm_cl = entry.val;
      _mm_clwb(nvm_cl);
    }
  }
#else
  while(!log.is_empty()) {
    auto entry = log.pop();
    cacheline_t* nvm_cl = add_addr(entry.addr, gv::NVM_config->offset());
    *nvm_cl = entry.val;
    _mm_clwb(nvm_cl);
  }
#endif
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
#ifdef SAVE_ALLOCATOR
  run_checkpointing_cpallocator();
#endif /* SAVE_ALLOCATOR */
  
  if(is_unit)
    nvm_heap_update_unit();
  else
    nvm_heap_update_multi();

#ifdef SAVE_ALLOCATOR
  join_cpallocator();
#endif /* SAVE_ALLOCATOR */
}

// cpu_bind() suppose NVM is
// connected with first socket.
// If NVM is connected with second socket,
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
  set_global_epoch(epoch);
  _mm_mfence();

  log_persistence();

  _mm_sfence();
  epoch = set_phase(epoch, epoch_phase::NVM_Heap_Update);
  set_global_epoch(epoch);
  _mm_mfence();

  nvm_heap_update();
}

void Escort::cpmaster_t::checkpointing() {
  cpu_bind(); // bind cpu connected NVM directly
  epoch_t epoch = Epoch(get_global_epoch());
  while(gv::isPersisting) {
    DEBUG_PRINT("checkpionting", epoch);
    while(!has_execute_right()) { /* wait */ }
    auto time_start = std::chrono::system_clock::now();
    
    _mm_sfence();
    epoch = set_phase(epoch+1, epoch_phase::Multi_Epoch_Existence);
    set_global_epoch(epoch);
    _mm_mfence();

    wait_previous_epoch_transactions(epoch);
    
    epoch = set_phase(epoch, epoch_phase::Log_Persistence);
    set_global_epoch(epoch);

    run_deallocator(); // for deallocation of delayed free operations

    debug::timer_start();
    log_persistence();
    debug::timer_end(debug::time_log_persistence);
    
    _mm_sfence();
    epoch = set_phase(epoch, epoch_phase::NVM_Heap_Update);
    set_global_epoch(epoch);
    _mm_mfence();

    debug::timer_start();
    nvm_heap_update();
    debug::timer_end(debug::time_nvm_heap_update);
    
    join_deallocator(); // join deallocator 
    
    release_execute_right();
    
    auto time_end   = std::chrono::system_clock::now();
    auto checkpointing_time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count());
    debug::time_checkpointing += checkpointing_time;
    if(checkpointing_time < gv::EpochLength)
      std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<std::uint64_t>(gv::EpochLength - checkpointing_time)));
    while(_num_wait_threads.load(std::memory_order_acquire) > 0) { /* wait */ }
  }

  debug::timer_ave(epoch);
  debug::plog_ave(epoch);
  finalize(epoch);
}

void Escort::cpmaster_t::init_userthreadctx(userthreadctx_t* ctx) {
  _num_wait_threads.fetch_add(1);

  while(!has_execute_right()) { /* wait */ }
  
  DEBUG_PRINT("init user");
  
  _ctx_array.push_back(ctx);
#ifndef ALLOCATOR_RALLOC
  gv::NVM_config->add_user_num();
#endif // ALLOCATOR_RALLOC

  _num_wait_threads.fetch_sub(1);
  release_execute_right();
}

void Escort::cpmaster_t::finalize_userthreadctx(userthreadctx_t* ctx) {
  _num_wait_threads.fetch_add(1);
  while(!has_execute_right()) { /* wait */ }
  
  DEBUG_PRINT("finalize user");
  epoch_t epoch = get_global_epoch();
  auto id = static_cast<uint32_t>((epoch-2) % 4);
  
  _ctx_delete_array[id].push_back(ctx);

  _num_wait_threads.fetch_sub(1);
  release_execute_right();
}
