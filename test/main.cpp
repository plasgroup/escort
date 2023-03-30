#include "../src/Escort.hpp"
#include "./list.hpp"
#include "./test.hpp"

int main(int argc, char* argv[]) {
  if(argc != 2)
    std::cerr << "The execution format is different" << std::endl;

  bool is_recovery = escort_is_recovery("/mnt/pmem/test");
  escort_init("/mnt/pmem/test", 1, is_recovery);
  escort_thread_init();

  int op = atoi(argv[1]);
  switch(op) {
  case 0:
    exec_list_single();
    break;
  case 1:
    exec_list_multi(MULTI_THREAD_NUM);
    break;
  case 2:
    exec_list_sort();
    break;
  }
  escort_thread_finalize();
  escort_finalize();
}
