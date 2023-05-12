#ifdef ALLOCATOR_RALLOC

#include "../ralloc/src/ralloc.hpp"
#include "allocator.hpp"
#include <sys/mman.h>

namespace Escort {
    void* nvm_base;
    void* dram_base;
    uint64_t sb_region_size;
    uint64_t nvm_dram_distance;  // DRAM - NVM
};
using namespace Escort;

static void* map_replica_region(uint64_t sb_region_size, void* map_addr)
{
    int flags =  MAP_PRIVATE|MAP_ANONYMOUS;
    if (map_addr != nullptr)
        flags |= MAP_FIXED;
    void* res = mmap(map_addr, sb_region_size, PROT_READ|PROT_WRITE, flags, -1, 0);
    if (res == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    return res;
}

int Escort::pm_init(const char* id, uint64_t size, void* dram_recovery_base)
{
    int res = RP_init(id, size);
    void* nvm_end;
    RP_region_range(SB_IDX, &nvm_base, &nvm_end);
    sb_region_size = ((uintptr_t) nvm_end) - ((uintptr_t) nvm_base);
    dram_base = map_replica_region(sb_region_size, dram_recovery_base);
    nvm_dram_distance = ((uintptr_t) dram_base) - ((uintptr_t) nvm_base);
    printf("NVM  %p\n", nvm_base);
    printf("DRAM %p\n", dram_base);
    return res;
}

void* Escort::pm_malloc(size_t size)
{
    size = (size + CACHELINE_SIZE - 1) & ~(CACHELINE_SIZE - 1);
    void* nvm_ptr = RP_malloc(size);
    return nvm_to_dram(nvm_ptr);
}

void Escort::pm_free(void* dram_ptr)
{
    void* nvm_ptr = dram_to_nvm(dram_ptr);
    RP_free(nvm_ptr);
}

void* Escort::pm_set_root(void* dram_ptr, uint64_t i)
{
    void* nvm_ptr = dram_to_nvm(dram_ptr);
    void* old_nvm_ptr = RP_set_root(nvm_ptr, i);
    return nvm_to_dram(old_nvm_ptr);
}



#elif defined(ALLOCATOR_JEMALLOC)

#endif