#ifndef ADDRLIST_T_HPP
#define ADDRLIST_T_HPP

#include <utility>
#include <vector>
#include <cassert>
#include "globalEscort.hpp"

#ifndef OLD_VERSION
class Escort::addrlist_t {
private:
  std::vector<std::pair<void*, std::size_t>> _list; // {addr, num_bits}
  
public:
  addrlist_t() {}
  void append(std::pair<void*, std::size_t> entry) {
    _list.push_back(entry);
  }

  const std::size_t size() const { return _list.size(); }
  bool empty() const { return (_list.size() == 0); }
  std::pair<void*, std::size_t> pop() {
    auto ret = _list[_list.size()-1];
    _list.pop_back();
    return ret;
  }

  inline void clear() {
    _list.clear();
  }

  inline const std::vector<std::pair<void*, std::size_t>>& array() const {
    return _list;
  }
  
  using iterator = std::vector<std::pair<void*, std::size_t>>::iterator;
  using const_iterator = std::vector<std::pair<void*, std::size_t>>::const_iterator;
  iterator begin() {
    return _list.begin();
  }
  const_iterator begin() const {
    return _list.begin();
  }
  iterator end() {
    return _list.end();
  }
  const_iterator end() const {
    return _list.end();
  }
};
#else
class Escort::addrlist_t {
public:
  void **_list;
  std::size_t _size;
  std::size_t _max_size = LOG_SIZE;
  const std::size_t _margin = 4000;

  addrlist_t() {
    _list = reinterpret_cast<void**>(std::malloc(sizeof(void*)*LOG_SIZE));
    for(int i = 0; i < LOG_SIZE; i++)
      _list[i] = nullptr;
    _size = 0;
    _max_size = LOG_SIZE;
  }
  
  inline void append(void* addr) {
    assert(_size < _max_size);
    _list[_size] = addr;
    _size++;
  }
  inline void* pop() {
    assert(_size > 0);
    _size--;
    return _list[_size];
  }
  inline bool is_empty() const { return (_size == 0); }
  inline const std::size_t size() const { return _size; }
};
#endif
#endif
