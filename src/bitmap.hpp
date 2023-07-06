#ifndef BITMAP_HPP
#define BITMAP_HPP

#include <atomic>

namespace Escort {

template<int NELEMS>
class Bitmap {
    using word_t = uint64_t;
    static const int BITS_IN_WORD = sizeof(word_t) * 8;
    static const int NWORDS = (NELEMS + BITS_IN_WORD - 1) / BITS_IN_WORD;
    std::atomic<word_t> bits[NWORDS];

public:

    void clear_all() {
        for (int i = 0; i < NWORDS; i++)
            bits[i].store(0, std::memory_order_relaxed);
    }

    // returns true if the bit was set
    bool set(int i) {
        int index = i / BITS_IN_WORD;
        int shift = i % BITS_IN_WORD;
        word_t mask = word_t(1) << shift;
        word_t old = bits[index].fetch_or(mask);
        return (old & mask) == mask;
    }

    // returns true if the bit was set
    bool clear(int i) {
        int index = i / BITS_IN_WORD;
        int shift = i % BITS_IN_WORD;
        word_t mask = word_t(1) << shift;
        word_t old = bits[index].fetch_and(~mask);
        return (old & mask) == mask;
    }

    int find_first_zero(int i = 0, bool set_if_found = true) {
        int index = i / BITS_IN_WORD;
        int shift = i % BITS_IN_WORD;
        for (; index < NWORDS; index++, shift = 0) {
            word_t x = bits[index].load(std::memory_order_relaxed);
            if ((x | ((word_t(1) << shift) - 1)) != ~word_t(0))
                for (word_t msk = word_t(1) << shift; shift < BITS_IN_WORD; msk <<= 1, shift++) {
                    if ((x & msk) == 0) {
                        int i = index * BITS_IN_WORD + shift;
                        if (i >= NELEMS)
                            return -1;
                        if (!(set_if_found && set(i)))
                            return i;
                    }
                }
        }
        return -1;
    }
};

}

#endif // BITMAP_HPP