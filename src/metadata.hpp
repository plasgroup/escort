#ifndef _METADATA_HPP__
#define _METADATA_HPP__

#include <stdint.h>
#include <x86intrin.h>

#include "root.hpp"

// old header file
#include "globalEscort.hpp"

namespace Escort {

class PersistentMetaData {
    int fd;
    uint64_t map_size;
    uintptr_t map_addr;

    static uint64_t round_up(uint64_t x) {
        return (x + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);
    }

    const uint64_t MAX_ROOTS = 1000;

    const uint64_t MAGIC_OFFSET = 0;
    const uint64_t DRAM_BASE_OFFSET = 8;
    const uint64_t EPOCH_OFFSET = 16;
    const uint64_t ROOT_AREA_OFFSET = 64;
    const uint64_t PLOG_AREA_OFFSET = ROOT_AREA_OFFSET + round_up(sizeof(PersistentRoots));

    const uint64_t MAGIC = 0x0101010101010101;
    
public:
    PersistentMetaData() : fd(-1), map_size(0), map_addr(0) {}
    ~PersistentMetaData() {
        if (map_addr != 0)
            close();
    }

    int open(const char* filename);
    void close();

    bool is_valid() {
        return get_magic() == MAGIC;
    }

    void init(void* dram_base);

    uintptr_t get_plog_area_addr() {
        return map_addr + PLOG_AREA_OFFSET;
    }

    uint64_t get_plog_area_size() {
        return map_size - PLOG_AREA_OFFSET;
    }
    
#define DEFINE_ACCESSOR(T, N, O)    \
    T get_##N() {   \
        return *reinterpret_cast<T*>(map_addr + O); \
    }   \
    void set_##N(T v, bool fence = true) {  \
        T* ptr = reinterpret_cast<T*>(map_addr + O);    \
        *ptr = v;   \
        _mm_clwb(ptr);  \
        if (fence)  \
            _mm_sfence();    \
    }

#define DEFINE_REF_GETTER(T, N, O) \
    T& N() { \
        return *reinterpret_cast<T*>(map_addr + O); \
    }

    DEFINE_ACCESSOR(uint64_t, magic, MAGIC_OFFSET)
    DEFINE_ACCESSOR(void*, dram_base, DRAM_BASE_OFFSET)
    DEFINE_ACCESSOR(epoch_t, epoch, EPOCH_OFFSET)
    DEFINE_REF_GETTER(PersistentRoots, roots, ROOT_AREA_OFFSET)
};

#ifdef ALLOCATOR_RALLOC
static inline epoch_t get_global_epoch() {
    return persistent_metadata->get_epoch();
}

static inline void set_global_epoch(epoch_t epoch) {
    persistent_metadata->set_epoch(epoch);
}
#endif // ALLOCATOR_RALLOC

}

#endif // _METADATA_HPP__