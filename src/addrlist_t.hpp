#ifndef ADDRLIST_T_HPP
#define ADDRLIST_T_HPP

#include <utility>
#include <vector>
#include "globalEscort.hpp"

class Escort::addrlist_t {
private:
  std::vector<std::pair<void*, std::size_t>> _list;
  
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

#endif
