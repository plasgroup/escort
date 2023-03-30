#ifndef LIST_HPP
#define LIST_HPP

#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

#include "../src/Escort.hpp"

class Node {
private:
  Node* _next;
  int32_t _val;
public:
  Node(int32_t value = 0) : _next(nullptr), _val(value) {
    escort_write_region(&_val, sizeof(int32_t));
  }
  ~Node() {}
  inline void set_val(int32_t value) {
    escort_write_region(&_val, sizeof(int32_t));
    _val = value;
  }
  inline int32_t get_val() const { return _val; }
  inline void set_next(Node* next) {
    escort_write_region(&_next, sizeof(Node*));
    _next = next;
  }
  inline Node* get_next() const { return _next; }
};

std::size_t get_list_size(Node* list);
Node* insert(Node* list);
Node* insert_sort(Node* list);
#endif
