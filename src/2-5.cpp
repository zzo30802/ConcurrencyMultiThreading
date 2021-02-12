#include <assert.h>
#include <stdio.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>  // std::accumulate
#include <string>
#include <thread>
#include <vector>

/**
 *  0 : get std::thread::id
 *  1 : join(), detach(), std::thread::id
**/
#define test_idx 1

#if test_idx == 0
void foo() {
  std::thread::id tid = std::this_thread::get_id();
  std::cout << "foo thread id : " << tid << std::endl;  // 15772
}

int main() {
  std::thread t1(foo);
  // get foo() thread id
  std::thread::id tid = t1.get_id();
  // get main thread id
  std::thread::id mainTid = std::this_thread::get_id();

  if (t1.joinable())
    t1.join();

  std::cout << "t1 tid : " << tid << std::endl;        // 15772
  std::cout << "main tid : " << mainTid << std::endl;  // 2036
  return 0;
}

#elif test_idx == 1
void foo() {
  std::cout << "foo start" << std::endl;
  std::thread::id tid = std::this_thread::get_id();
  std::cout << "foo thread id : " << tid << std::endl;  // 5760
  std::cout << "foo end" << std::endl;
}

int main() {
  std::thread t1(foo);  // 5760
  std::thread::id tid = t1.get_id();
  if (t1.joinable()) {
    t1.join();
  }
  std::cout << "Thread from Main : " << tid << std::endl;  // 5760

  std::thread t2(foo);  // 9444
  t2.detach();
  std::thread::id tid2 = t2.get_id();
  assert(tid2 == std::thread::id());

  std::cout << "sleep 2s\n";
  std::this_thread::sleep_for(std::chrono::seconds(2));

  std::cout << "Thread from Main : " << tid2 << std::endl;  // 0

  return 0;
}
#endif