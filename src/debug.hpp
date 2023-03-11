#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "globalEscort.hpp"

#include <iostream>
#include <utility>
#include <cstdlib>

namespace Escort {
  namespace debug {
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
