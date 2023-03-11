#ifndef ROOTTABLE_T_HPP
#define ROOTTABLE_T_HPP

#include <mutex>
#include <atomic>
#include <cstring>
#include <cassert>

#include "Escort.hpp"
#include "globalEscort.hpp"
#include "debug.hpp"
#include "userthreadctx_t.hpp"

namespace lv = Escort::LocalVariable;

#define BUCKET_SIZE 512

class Escort::roottable_t {
public:
  class Node {
  public:
    char  id[CACHE_LINE_SIZE];
    void* addr;
    Node* next = nullptr;
    Node() {}
    Node(const char* rootid, void* rootaddr) {
      lv::ctx->mark(&id, CACHE_LINE_SIZE);
      lv::ctx->mark(&addr, sizeof(void*));
      memcpy(id, rootid, strlen(rootid));
      addr = rootaddr;
    }
    void update(void* addr) {
      lv::ctx->mark(&this->addr, sizeof(addr));
      this->addr = addr;
    }
  };
  class Bucket {
  public:
    std::atomic<bool> _lock;
  public:    
    Node *head;

    void lock() {
      bool expected = false;
      while(!_lock.compare_exchange_weak(expected, true)) {} // busy wait
    }
    void unlock() {
      bool expected = true;
      bool is_unlock = _lock.compare_exchange_weak(expected, false);
      if(!is_unlock) {
	DEBUG_ERROR("bucket unlock error");
      }
    }
  } __attribute__((aligned(CACHE_LINE_SIZE)));
private:
  std::uint32_t hashfn(const char* rootid) { // should modify
    std::size_t id_len = strlen(rootid);
    std::uint32_t ret = 0;
    for(int i = 0; i < id_len; i++)
      ret += static_cast<std::uint32_t>(rootid[i]);
    return ret;
  }
public:
  Bucket buckets[BUCKET_SIZE];
  roottable_t() {
    for(int i = 0; i < BUCKET_SIZE; i++)
      buckets[i].head = nullptr;
  }
  void set(const char* rootid, void* rootaddr) {
    std::size_t bucket_id = hashfn(rootid);
    Bucket &bucket = buckets[bucket_id];
    bucket.lock();
    
    Node* curr = bucket.head;
    while(curr != nullptr) {
      if(strcmp(curr->id, rootid) == 0) { // exist
	curr->update(rootaddr);
	bucket.unlock();
	return;
      }
      curr = curr->next;
    }
    Node* new_node = PNEW(Node, rootid, rootaddr);
    if(bucket.head == nullptr) {
      lv::ctx->mark(&bucket.head, sizeof(bucket.head));
      bucket.head = new_node;
    } else {
      while(curr->next != nullptr)
	curr = curr->next;
      lv::ctx->mark(&curr->next, sizeof(curr->next));
      curr->next = new_node;
    }
    bucket.unlock();
  }

  void* get(const char* rootid) {
    std::size_t bucket_id = hashfn(rootid);
    Bucket &bucket = buckets[bucket_id];
    Node* curr = bucket.head;
    while(curr != nullptr) {
      if(strcmp(curr->id, rootid) == 0)
	return curr->addr;
      curr = curr->next;
    }
    return nullptr;
  }
  
  void remove(const char* rootid) {
    std::size_t bucket_id = hashfn(rootid);
    Bucket &bucket = buckets[bucket_id];
    assert(bucket.head != nullptr);

    bucket.lock();
   
    Node* remove_node = nullptr;

    if(strcmp(bucket.head->id, rootid) == 0) {
      lv::ctx->mark(&bucket.head, sizeof(bucket.head));
      remove_node = bucket.head;
      bucket.head = bucket.head->next;
    }
    
    Node* curr = bucket.head;    
    while(curr->next != nullptr) {
      if(strcmp(curr->next->id, rootid) == 0) {
	remove_node = curr->next;
	lv::ctx->mark(&curr->next, sizeof(curr->next));
	curr->next = curr->next->next;
      }
    }
    if(remove_node != nullptr)
      PDELETE(remove_node);
    else
      DEBUG_ERROR("id is not found", rootid);

    bucket.unlock();
  }
};
#endif
