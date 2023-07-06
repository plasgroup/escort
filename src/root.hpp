#ifndef ROOT_HPP
#define ROOT_HPP

#include <stdint.h>
#include <unistd.h>
#include <x86intrin.h>

#include <atomic>

#include "../ralloc/src/ralloc.hpp"
#include "allocator.hpp"
#include "bitmap.hpp"

#include "globalEscort.hpp"

namespace Escort {
class PersistentRoots {
    using raid_t = int16_t;  // ID of ralloc
    static const raid_t INVALID = -1;

    static const uint64_t RALLOC_MAX_ROOTS = ::MAX_ROOTS;
    static const uint64_t MAX_ROOTS = RALLOC_MAX_ROOTS >> 2;

    struct root_array {
        std::atomic<raid_t> ralloc_id[MAX_ROOTS];
    };
    root_array rootsets[4];    // persistent

    int rootset_index(int epoch) {
        return (epoch + 4) & 3;
    }

    std::atomic<raid_t>& get_raid(int epoch, int id) {
        int index = rootset_index(epoch);
        return rootsets[index].ralloc_id[id];
    }

    void make_persistent(int epoch) {
        int index = rootset_index(epoch);
        for (uintptr_t p = reinterpret_cast<uintptr_t>(&rootsets[index].ralloc_id[0]);
                p < reinterpret_cast<uintptr_t>(&rootsets[index].ralloc_id[MAX_ROOTS]);
                p += CACHE_LINE_SIZE)
            _mm_clwb(reinterpret_cast<void*>(p));              
    }

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

    void init() {
        for (int j = 0; j < 4; j++)
            for (int i = 0; i < MAX_ROOTS; i++)
                rootsets[j].ralloc_id[i].store(INVALID, std::memory_order_relaxed);
        used_ralloc_ids.clear_all();
    }

    // in transaction or sequential
    // [thread local] epoch is given
    void *get_root(int epoch, int id) {
        raid_t raid = get_raid(epoch + 1, id).load(std::memory_order_relaxed);
        if (raid == INVALID)
            raid = get_raid(epoch, id).load(std::memory_order_relaxed);
        if (raid != INVALID)
            return pm_get_root<char>(raid);
        return nullptr;
    }

    // in transaction or sequential
    // [thread local] epoch is given
    void set_root(int epoch, bool prepare_phase, int id, void* ptr) {
        std::atomic<raid_t>& curr = get_raid(epoch + 1, id);
        if (curr.load(std::memory_order_relaxed) == -1 || prepare_phase) {
            raid_t raid = allocate_ralloc_id();
            pm_set_root(ptr, raid);
            curr.store(raid, std::memory_order_relaxed);
        } else
            pm_set_root(ptr, curr.load(std::memory_order_relaxed));
    }

    // recover the given [persistent epoch]
    //   epoch persistent (in use)
    //   +1    RO or persistent -> persistent
    //   +2    RW or RO         -> RW
    //   +3    ?                -> clean
    void recovery(int epoch) {
        used_ralloc_ids.clear_all();
        for (int i = 0; i < MAX_ROOTS; i++) {
            raid_t raid = get_raid(epoch, i).load(std::memory_order_relaxed);
            get_raid(epoch + 1, i).store(raid, std::memory_order_relaxed);
            get_raid(epoch + 2, i).store(raid, std::memory_order_relaxed);
            get_raid(epoch + 3, i).store(INVALID, std::memory_order_relaxed);
            used_ralloc_ids.set(raid);
            pm_get_root<char>(raid);
        }
        make_persistent(epoch + 1);
    }

    // call in persistent phase
    //   -2    persistent (in use)
    //   -1    persistent
    //   epoch RO
    //   +1    RW partial -> RW total
    void advance(int epoch) {
        int curr_index = epoch & 3;
        int prev_index = (epoch + 3) & 3;
        for (int i = 0; i < MAX_ROOTS; i++) {
            raid_t prev = get_raid(epoch, i).load(std::memory_order_relaxed);
            if (prev != INVALID) {
                raid_t curr = get_raid(epoch + 1, i).load(std::memory_order_relaxed);
                if (curr == INVALID)
                    get_raid(epoch + 1, i).compare_exchange_weak(curr, prev);
            }
        }
    }

    // call in release phase
    //   -2    unused       -> clean
    //   -1    persistent
    //   epoch RO           -> persistent
    //   +1    RW total
    void release(int epoch) {
        for (int i = 0; i < MAX_ROOTS; i++) {
            std::atomic<raid_t>& to_release = get_raid(epoch - 2, i);
            raid_t raid = to_release.load(std::memory_order_relaxed);
            if (raid != INVALID) {
                raid_t next = get_raid(epoch - 1, i).load(std::memory_order_relaxed);
                if (raid != next)
                    free_ralloc_id(raid);
                to_release.store(INVALID, std::memory_order_relaxed);
            }
        }
        make_persistent(epoch);
    }
};
}

#endif // ROOT_HPP