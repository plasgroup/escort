#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

#include "./list.hpp"

std::size_t get_list_size(Node* list) {
  std::size_t list_size = 0;
  while(list != nullptr) {
    list = list->get_next();
    list_size++;
  }
  return list_size;
}

void insert(Node* list) {
  Node* new_node = PNEW(Node, 1);
  new_node->set_next(list);
  list = new_node;
  Escort_set_root("list", list);
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
