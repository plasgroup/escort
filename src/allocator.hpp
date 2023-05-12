
#ifdef ALLOCATOR_RALLOC

#include <stdint.h>
#include <unistd.h>

namespace Escort {
    extern void* nvm_base;
    extern void* dram_base;
    extern uint64_t sb_region_size;
    extern uint64_t nvm_dram_distance;  // DRAM - NVM

    void* dram_to_nvm(void* dram_ptr);
    void* nvm_to_dram(void* nvm_ptr);
    int pm_init(const char* id, uint64_t size);
    void* pm_malloc(size_t size);
    void pm_free(void* dram_ptr);
    template<class T> T* pm_get_root(uint64_t i);
    void* pm_set_root(void* dram_ptr, uint64_t i);
}

void* Escort::dram_to_nvm(void* dram_ptr)
{
    if (dram_ptr == nullptr)
        return nullptr;
    uintptr_t dram_addr = (uintptr_t) dram_ptr;
    return (void*) (dram_addr - nvm_dram_distance);
}

void* Escort::nvm_to_dram(void* nvm_ptr)
{
    if (nvm_ptr == nullptr)
        return nullptr;
    uintptr_t nvm_addr = (uintptr_t) nvm_ptr;
    return (void*) (nvm_addr + nvm_dram_distance);
}

template<class T>
T* Escort::pm_get_root(uint64_t i)
{
    T* nvm_ptr = RP_get_root<T>(i);
    return reinterpret_cast<T*>(nvm_to_dram(nvm_ptr));
}

#elif defined(ALLOCATOR_JEMALLOC)

inline void* pm_malloc(size_t size) {
    return _escort_internal_malloc(size);
}
inline void pm_free(void* ptr) {
    _escort_internal_free(ptr);
}

#else
#error no PM allocator is specified
#endif
