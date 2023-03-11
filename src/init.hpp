#ifndef INIT_HPP
#define INIT_HPP

#include "Escort.hpp"
#include "globalEscort.hpp"

namespace Escort {
  namespace init {
    void init_roottable();
    void init_bitmap();
    void init_manager();
    void init_checkpointing_thread(std::uint32_t checkpointing_num);
  }
}

#endif
