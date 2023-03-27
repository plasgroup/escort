#ifndef USERTHREADCTX_HPP
#define USERTHREADCTX_HPP

#include <atomic>

#include "globalEscort.hpp"
#include "addrlist_t.hpp"
#include "plog_t.hpp"
#include "allocatorlog_t.hpp"

namespace gv = Escort::GlobalVariable;

namespace Escort {
  class userthreadctx_t {
  private:
    std::atomic<uint64_t> _epoch;
    addrlist_t *_addrlist[2];         // PLACE ON DRAM
    plog_t *_log;                     // PLACE ON NVM, _log is manager of log contents
    allocatorlog_t *_allocatorlog[2]; // PLACE ON NVM, _allocatorlog is manager of log contents
    std::list<void*> _delay_dealloc_list[2];
  private:
    void* malloc_with_create_log(std::size_t size);
    void free_with_create_log(void* addr, std::size_t size);
  public:
    userthreadctx_t() {
      for(int i = 0; i < 2; i++) {
	_addrlist[i] = NEW(addrlist_t);
	_allocatorlog[i] = NEW(allocatorlog_t);
      }
      _log = NEW(plog_t);
    }
    ~userthreadctx_t() {}

    inline std::uint64_t get_epoch(std::memory_order order = std::memory_order_seq_cst) {
      return _epoch.load(order);
    }
    inline void set_epoch(std::uint64_t epoch, std::memory_order order = std::memory_order_seq_cst) {
      _epoch.store(epoch, order);
    }

    // called by funcions in Escort.cpp
    void* malloc(std::size_t size);
    void  free(void* addr, std::size_t size);
    void  begin_op();
    void  end_op();
    void  mark(void* addr, std::size_t size);
    void  set_root(const char* id, void* addr);
    void* get_root(const char* id);
    void  remove_root(const char* id);

    // for checkpointing thread
    inline addrlist_t& list(bool id) const { return *_addrlist[id]; }
    inline plog_t& log() const { return *_log; }
    inline allocatorlog_t& allocatorlog(bool id = 0) const { return *_allocatorlog[id]; }
    inline const std::list<void*>& dealloc_list(bool id = 0) const { return _delay_dealloc_list[id]; }
    inline void clear_dealloc_list(bool id = 0) {
      _delay_dealloc_list[id].clear();
    }
  };
}

#endif
