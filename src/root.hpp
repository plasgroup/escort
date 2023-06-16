#include <stdint.h>
#include <unistd.h>
#include <x86intrin.h>

#include <atomic>

#include "ralloc.hpp"
#include "allocator.hpp"
#include "bitmap.hpp"

namespace Escort {
class PersistentRoots {
    static const uint64_t RALLOC_MAX_ROOTS = ::MAX_ROOTS;
    static const uint64_t MAX_ROOTS = RALLOC_MAX_ROOTS >> 2;
    struct root_array {
        std::atomic<int16_t> ralloc_id[MAX_ROOTS];
    };
    root_array roots[4];    // persistent

    Bitmap<RALLOC_MAX_ROOTS> used_ralloc_ids;
    static_assert(RALLOC_MAX_ROOTS < (1 << (sizeof(uint16_t) << 3)));

    uint16_t allocate_ralloc_id() {
        int x = used_ralloc_ids.find_first_zero(0, true);
        assert(x != -1);
        return (uint16_t) x;
    }

    void free_ralloc_id(uint16_t rid) {
        used_ralloc_ids.clear(rid);
    }

    public:

    void *get_root(int epoch, int id) {
        int curr_index = epoch & 3;
        int prev_index = (epoch + 3) & 3;
        if (roots[curr_index].ralloc_id[id] != -1)
            return pm_get_root<char>(roots[curr_index].ralloc_id[id]);
        else if (roots[prev_index][id] != -1)
            return pm_get_root<char>(roots[prev_index].ralloc_id[id]);
        return nullptr;
    }

    void set_root(int epoch, bool async_phase, int id, void* ptr) {
        int curr_index = epoch & 3;
        if (roots[curr_index].ralloc_id[id] == -1 || async_phase) {
            int ralloc_id = allocate_ralloc_id();
            pm_set_root(ptr, ralloc_id);
            roots[curr_index].ralloc_id[id] = ralloc_id;
        } else
            pm_set_root(ptr, roots[curr_index].ralloc_id[id]);
    }

    // recover the given epoch
    void recovery(int epoch) {
        int curr_index = epoch & 3;
        // clear other epochs
        for (int j = 0; j < 4; j++)
            if (j != curr_index)
                for (int i = 0; i < MAX_ROOTS; i++)
                    roots[j].ralloc_id[i] = -1;
        // notify ralloc, and
        // set up bitmap
        used_ralloc_ids.clear_all();
        for (int i = 0; i < MAX_ROOTS; i++) {
            uint16_t rid = roots[curr_index].ralloc_id[i]; 
            used_ralloc_ids.set(rid);
            pm_get_root<char>(rid);
        }
    }

    // call in persistent phase
    //   prev prev epoch is persistent
    //   prev epoch is read only
    void advance(int epoch) {
        int curr_index = epoch & 3;
        int prev_index = (epoch + 3) & 3;
        for (int i = 0; i < MAX_ROOTS; i++) {
            uint16_t curr = roots[curr_index].ralloc_id[i];
            uint16_t prev = roots[prev_index].ralloc_id[i];
            if (curr == -1 && prev != -1)
                roots[curr_index].ralloc_id[i].compare_exchange_strong(curr, prev);
        }
    }

    // call in replay phase
    //   prev prev epoch is unused <-- release this
    //   prev epoch is persistent
    void release_old(int epoch) {
        int prev_index = (epoch + 3) & 3;
        int prev_prev_index = (epoch + 2) & 3;
        for (int i = 0; i < MAX_ROOTS; i++) {
            uint16_t prev_prev = roots[prev_prev_index].ralloc_id[i];
            if (prev_prev != -1)
                if (roots[prev_index].ralloc_id[i] != prev_prev)
                    free_ralloc_id(prev_prev);
        }
        for (int i = 0; i < MAX_ROOTS; i++)
            roots[prev_prev_index].ralloc_id[i] = -1;
    }

    // call in inactive phase
    //  prev epoch is persistent
    void prepare(int epoch) {
        int prev_index = (epoch + 3) & 3;
        for (uintptr_t p = reinterpret_cast<uintptr_t>(&roots[prev_index].ralloc_id[0]);
                p < reinterpret_cast<uintptr_t>(&roots[prev_index].ralloc_id[MAX_ROOTS]);
                p += CACHE_LINE_SIZE)
            _mm_clwb(reinterpret_cast<void*>(p));              
    }
};
}