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
#ifdef SAVE_ALLOCATOR
    allocatorlog_t *_allocatorlog[2]; // PLACE ON NVM, _allocatorlog is manager of log contents
#endif /* SAVE_ALLOCATOR */
    std::list<void*> _delay_dealloc_list[2];
  private:
    void* malloc_with_create_log(std::size_t size);
    void free_with_create_log(void* addr, std::size_t size);
#ifdef ESCORT_PROF
  public:
    int _delay_dealloc_list_size[2] = {0, 0};
#endif /* ESCORT_PROF */
  public:
    userthreadctx_t() : _epoch(0) {
      for(int i = 0; i < 2; i++) {
	_addrlist[i] = NEW(addrlist_t);
#ifdef SAVE_ALLOCATOR
	_allocatorlog[i] = NEW(allocatorlog_t);
#endif /* SAVE_ALLOCATOR */
      }
#ifndef OLD_VERSION
      _log = NEW(plog_t);
#else
      _log = reinterpret_cast<plog_t*>(gv::NVM_config->malloc(sizeof(plog_t)));
      new(_log) plog_t();
#endif
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
#ifndef OLD_VERSION
    inline addrlist_t& list(bool id) const { return *_addrlist[id]; }
#else
    inline addrlist_t* list(bool id) const { return _addrlist[id]; }
#endif
    inline plog_t& log() const { return *_log; }
#ifdef SAVE_ALLOCATOR
    inline allocatorlog_t& allocatorlog(bool id = 0) const { return *_allocatorlog[id]; }
#endif /* SAVE_ALLOCATOR */
    inline const std::list<void*>& dealloc_list(bool id = 0) const { return _delay_dealloc_list[id]; }
    inline void clear_dealloc_list(bool id = 0) {
      _delay_dealloc_list[id].clear();
#ifdef ESCORT_PROF
      _delay_dealloc_list_size[id] = 0;
#endif /* ESCORT_PROF */
    }
  };
}

#endif
