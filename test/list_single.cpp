#include "../src/Escort.hpp"
#include "./list.hpp"
#include "./test.hpp"

void exec_list_single() {
  Escort_begin_op();
  Node* list = (Node*) Escort_get_root("list");
  Escort_end_op();
  if(list == nullptr) {
    for(int i = 0; i < INSERT_COUNT; i += TRANSACTION_NUM) {
      Escort_begin_op();
      for(int j = i; j < i + TRANSACTION_NUM; j++)
	insert(list);
      Escort_end_op();
    }
  } else {
    std::size_t list_size = get_list_size(list);
    if(list_size == 0)
      std::cerr << "list is empty" << std::endl;
    else if(list_size % TRANSACTION_NUM != 0)
      std::cerr << "recovery list fails: " << list_size << std::endl;

    for(int i = 0; i < INSERT_COUNT - list_size; i += TRANSACTION_NUM) {
      Escort_begin_op();
      for(int j = i; j < i + TRANSACTION_NUM; j++)
	insert(list);
      Escort_end_op();
    }	
  }
}
