#include "../src/Escort.hpp"
#include "./list.hpp"
#include "./test.hpp"

int main(int argc, char* argv[]) {
  if(argc != 2)
    std::cerr << "The execution format is different" << std::endl;

  bool is_recovery = Escort_is_recovery("/mnt/pmem/test");
  Escort_init("/mnt/pmem/test", 1, is_recovery);
  Escort_thread_init();

  int op = atoi(argv[1]);
  switch(op) {
  case 0:
    exec_list_single();
    break;
  }
  Escort_thread_finalize();
  Escort_finalize();
}
