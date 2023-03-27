#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "globalEscort.hpp"

#include <iostream>
#include <utility>
#include <cstdlib>
#include <chrono>
#include <atomic>

namespace Escort {
  namespace debug {
    extern std::chrono::system_clock::time_point time_start;
    extern double time_log_persistence, time_nvm_heap_update;
    extern double time_checkpointing;
    extern std::uint64_t num_plog;
    extern std::atomic<std::uint64_t> num_plog_user;
    
    void timer_start();
    void timer_end(double& sum_exec_time);
    void timer_ave(epoch_t epoch);
    void add_plog(std::size_t size);
    void add_plog_user();
    void plog_ave(epoch_t epoch);
    void print();
    void print_error();
    
    template<typename head, typename... Tail>
    void print(head&& msg, Tail&&... args) {
      std::cout << " " << msg;
      print(std::forward<Tail>(args)...);
    }

    template<typename head, typename... Tail>
    void print_error(head&& msg, Tail&&... args) {
      std::cerr << " " << msg;
      print_error(std::forward<Tail>(args)...);
    }

    template<typename head, typename... Tail>
    void print_first(head&& msg, Tail&&... args) {
      std::cout << msg;
      print(std::forward<Tail>(args)...);
    }

    template<typename head, typename... Tail>
    void print_error_first(head&& msg, Tail&&... args) {
      std::cerr <<  msg;
      print_error(std::forward<Tail>(args)...);
    }
  }
}


#ifdef DEBUG_MODE
#define DEBUG_PRINT(msg, ...) ({			\
      Escort::debug::print_first(msg, ##__VA_ARGS__);	\
    })
#ifdef DEBUG_STRICT
#define DEBUG_PRINT_STRICT(msg, ...) ({		\
//       Escort::debug::print(msg, ##__VA_ARGS__);	\
//       std::cout << __FILE__ << std::endl;	\
//     })
#else
#define DEBUG_PRINT_STRICT(msg, ...)
#endif
#else
#define DEBUG_PRINT(msg, ...)
#define DEBUG_PRINT_STRICT(msg, ...)
#endif

#define DEBUG_ERROR(msg, ...) ({					\
      Escort::debug::print_error_first(msg, ##__VA_ARGS__);		\
      std::cerr << __FILE__ << std::endl;				\
      std::quick_exit(EXIT_FAILURE);					\
    })

#endif
