#include <mutex>
#include <random>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "../src/Escort.hpp"
#include "./list.hpp"
#include "./test.hpp"

Node* global_list = nullptr;
std::mutex global_mtx;

void exec_each_thread_list(int insert_count) {
  escort_thread_init();
  assert(insert_count % TRANSACTION_NUM == 0);
  for(int i = 0; i < insert_count; i+= TRANSACTION_NUM) {
    global_mtx.lock();
    escort_begin_op();
    for(int j = i; j < i + TRANSACTION_NUM; j++)
      global_list = insert(global_list);
    global_mtx.unlock();
    escort_end_op();
  }
}

void exec_list_multi(int thread_num) {
  escort_begin_op();
  global_list = (Node*) escort_get_root("list");
  escort_end_op();
  if(global_list == nullptr) {
    std::thread threads[thread_num];
    for(int i = 0; i < thread_num; i++)
      threads[i] = std::thread(exec_each_thread_list, INSERT_COUNT);

#ifdef KILL_MODE
  pid_t my_pid = getpid();
  std::random_device rnd;
  int32_t sleep_time = rnd() % 10 + 5;
  sleep(sleep_time);
  kill(my_pid, SIGKILL);
#endif
    
    for(int i = 0; i < thread_num; i++)
      threads[i].join();

    escort_begin_op();
    auto result = global_list;
    global_list = (Node*) escort_get_root("list");
    escort_end_op();
    std::size_t list_size = get_list_size(global_list);
    if(list_size != INSERT_COUNT*thread_num)
      std::cout << "error: " << list_size << ", " << global_list << ", " << result << std::endl;
  } else {
    std::size_t list_size = get_list_size(global_list);
    if(list_size == 0)
      std::cerr << "list is empty" << std::endl;
    else if(list_size % TRANSACTION_NUM != 0)
      std::cerr << "recovery list fails: " << list_size << std::endl;
  }
}
