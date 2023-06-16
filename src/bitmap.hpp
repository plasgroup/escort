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

    // this method is not lock-free
    // returns true if the bit was set
    bool set(int i) {
        int index = i / BITS_IN_WORD;
        int shift = i % BITS_IN_WORD;
        word_t mask = word_t(1) << shift;
        word_t old = bits[index].load();
        while ((old & mask) == 0) {
            if (bits[index].compare_exchange_strong(old, old |= mask))
                return false;
        }
        return true;
    }

    // this method is not lock-free
    // returns true if the bit was set
    bool clear(int i) {
        int index = i / BITS_IN_WORD;
        int shift = i % BITS_IN_WORD;
        word_t mask = word_t(1) << shift;
        word_t old = bits[index].load();
        while ((old & mask) != 0) {
            if (bits[index].compare_exchange_strong(old, old &= ~mask))
                return true;
        }
        return false;
    }

private:
    int find_first_zero_in_word(word_t x, int index, int shift, bool set_if_found) {
        assert(x != ~word_t(0));
        for (word_t p = word_t(1) << shift; (x & p) != 0; p <<= 1, shift++)
            ;
        int i = index * BITS_IN_WORD + shift;
        if (i >= NELEMS)
            return -1;
        if (set_if_found && set(i))
            return -1;
        return i;
    }

public:
    int find_first_zero(int i, bool set_if_found = false) {
        int index = i / BITS_IN_WORD;
        int shift = i % BITS_IN_WORD;
        for (; index < NWORDS; index++, shift = 0) {
            word_t x = bits[index].load(memory_order_relaxed);
            if (x | ((word_t(1) << shift) - 1) != ~word_t(0)) {
                int ret = find_first_zero_in_word(x, index, shift, set_if_found);
                if (ret != -1)
                    return ret;
            }
        }
        return -1;
    }
};

}