#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <random>

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

void insert_sort(Node* list) {
  std::random_device rnd;
  Node* new_node = PNEW(Node, rnd());

  if(list == nullptr) {
    list = new_node;
    Escort_set_root("list", list);
  } else if(list->get_val() >= new_node->get_val()) {
    new_node->set_next(list);
    list = new_node;
    Escort_set_root("list", list);
  } else {
    auto list_tmp = list;
    while(list_tmp->get_next() != nullptr) {
      if(list_tmp->get_next()->get_val() >= new_node->get_val())
	break;
      list_tmp = list_tmp->get_next();
    }
    new_node->set_next(list_tmp->get_next());
    list_tmp->set_next(new_node);    
  }
}
