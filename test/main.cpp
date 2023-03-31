#include "../src/Escort.hpp"
#include "./list.hpp"
#include "./test.hpp"

int main(int argc, char* argv[]) {
  if(argc != 3)
    std::cerr << "main <NVM file path> <test number>" << std::endl;

  char* nvm_file_path = argv[1];

  bool is_recovery = escort_is_recovery(nvm_file_path);
  escort_init(nvm_file_path, 1, is_recovery);
  escort_thread_init();

  int op = atoi(argv[2]);
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
