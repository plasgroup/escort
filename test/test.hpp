#ifndef TEST_HPP
#define TEST_HPP

#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

#define TRANSACTION_NUM 1000
#define INSERT_COUNT   10000
#define MULTI_THREAD_NUM   8

void exec_list_single();
void exec_list_multi(int thread_num);
void exec_list_sort();
#endif
