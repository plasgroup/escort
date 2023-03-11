#ifndef CHECKPOININTG_HPP
#define CHECKPOINTING_HPP

#include <atomic>
#include <vector>
#include <thread>

#include <hwloc.h>

#include "globalEscort.hpp"
#include "userthreadctx_t.hpp"
#include "plog_t.hpp"
#include "allocatorlog_t.hpp"
#include "deallocator_t.hpp"
#include "cpallocator_t.hpp"
#include "nvm/config.hpp"
#include "debug.hpp"

namespace gv = Escort::GlobalVariable;

class Escort::cpmaster_t {
public:
  class cpworker_t {
    plog_t *_log;
  public:
    cpworker_t() {
      _log = gv::NVM_config->pnew<plog_t>();
    }
  };
  
private:
  // worker
  cpworker_t **_workers = nullptr;
  // for cpu_bind
  hwloc_topology_t topology;
  hwloc_obj_t affinity = nullptr;
  // redo log
  plog_t *_log = nullptr;
  bool is_unit = false;
  // deallocator
  deallocator_t *_deallocator = nullptr;
  // checkpointing threads of allocator logs
  cpallocator_t *_cpallocator = nullptr;
  // user thread ctx;
  std::vector<userthreadctx_t*> _ctx_array;
  std::vector<userthreadctx_t*> _ctx_delete_array;
  std::atomic<bool> _has_execute_right;
  std::atomic<uint32_t> _num_wait_threads;
  std::thread _thread;
private:
  // for checkpointing
  void cpu_bind();
  void wait_previous_epoch_transactions(std::uint64_t epoch);
  void log_persistence_unit();
  void log_persistence_multi();
  void log_persistence();
  void replay_redo_log(plog_t& log);
  void clear_redo_logs();
  void nvm_heap_update_unit();
  void nvm_heap_update_multi();
  void nvm_heap_update();
  void finalize(epoch_t epoch);
  void checkpointing();

  // for delay deallocation
  inline void run_deallocator() {
    _deallocator->run(_ctx_array);
  }
  inline void join_deallocator() {
    _deallocator->join();
  }
  // for checkpointing of allocator log
  inline void run_check_valid_cpallocator() {
    _cpallocator->run_check_valid(_ctx_array);
  }
  inline void run_checkpointing_cpallocator() {
    _cpallocator->run_checkpointing(_ctx_array);
  }
  inline void join_cpallocator() {
    _cpallocator->join();
  }
  // for waiting user threads
  inline bool has_execute_right() {
    bool expected = false;
    return _has_execute_right.compare_exchange_weak(expected, true);    
  }
  inline void release_execute_right() { _has_execute_right.store(false, std::memory_order_release); }
  
public:
  cpmaster_t(std::uint32_t checkpointing_num = 1) {
    if(checkpointing_num == 1)
      is_unit = true;
    else
      is_unit = false;

    if(is_unit) {
      _log = NEW(plog_t);
    } else {    
      _workers = reinterpret_cast<cpworker_t**>(std::malloc(sizeof(cpworker_t*) * checkpointing_num));
      for(int i = 0; i < checkpointing_num; i++)
	_workers[i] = NEW(cpworker_t);
    }

    _deallocator = NEW(deallocator_t);
    _cpallocator = NEW(cpallocator_t);

    _has_execute_right.store(false, std::memory_order_release);
    _num_wait_threads.store(0, std::memory_order_release);
  }

  inline void run() {
    _thread = std::thread(&Escort::cpmaster_t::checkpointing, this);
  }
  inline void join() { _thread.join(); }
  void init_userthreadctx(userthreadctx_t* ctx);
  void finalize_userthreadctx(userthreadctx_t* ctx);
};

#endif
