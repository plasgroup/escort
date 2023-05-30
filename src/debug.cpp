#include "../config.h"

#include "debug.hpp"

namespace Escort {
  namespace debug {
    std::chrono::system_clock::time_point time_start;
    double time_log_persistence = 0, time_nvm_heap_update = 0;
    double time_checkpointing = 0;
    std::uint64_t num_plog = 0;
    std::atomic<std::uint64_t> num_plog_user(0);
    void timer_start() {
#ifdef MEASURE
      time_start = std::chrono::system_clock::now();
#endif
    }
    void timer_end(double& sum_exec_time) {
#ifdef MEASURE
      auto time_end = std::chrono::system_clock::now();
      auto duration_time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count());
      sum_exec_time += duration_time;
#endif
    }
    void timer_ave(epoch_t epoch) {
      time_log_persistence /= (epoch-2);
      time_nvm_heap_update /= (epoch-2);
      time_checkpointing   /= (epoch-2);
    }

    void add_plog(std::size_t size) {
#ifdef MEASURE
      num_plog += size;
#endif
    }

    void add_plog_user() {
#ifdef MEASURE
      num_plog_user += 1;
#endif
    }
    
    void plog_ave(epoch_t epoch) {
      num_plog  /= (epoch-2);
      num_plog_user = num_plog_user / (epoch-2);
    }
    
    void print() {
      std::cout << std::endl;
    }
    void print_error() {
      std::cout << std::endl;
    }
  }
}
