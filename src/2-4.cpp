#include <stdio.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>  // std::accumulate
#include <string>
#include <thread>
#include <vector>
/*
1 : 2-8 A naïve parallel version of std::accumulate(std::accumulate)
*/
#define test_idx 1
#if test_idx == 1
// std::accumulate
template <typename Iterator, typename T>
struct accumulate_block {
  void operator()(Iterator first, Iterator last, T& result) {
    result = std::accumulate(first, last, result);
  }
};

template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
  unsigned long const length = std::distance(first, last);
  // check input
  if (!length)
    return init;

  unsigned long const min_per_thread = 25;
  unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;

  unsigned long const hardware_threads = std::thread::hardware_concurrency();

  unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
  // The number of entries for each thread to process is the length of the range  divided by the number of threads
  unsigned long const block_size = length / num_threads;

  std::vector<T> results(num_threads);
  // 開啟thread的數量要-1，因為原本就已經有一個了
  std::vector<std::thread> threads(num_threads - 1);

  Iterator block_start = first;
  for (unsigned long i = 0; i < (num_threads - 1); ++i) {
    Iterator block_end = block_start;
    std::advance(block_end, block_size);

    // 開啟新的thread來累加計算結果
    threads[i] = std::thread(accumulate_block<Iterator, T>(),
                             block_start,
                             block_end,
                             std::ref(results[i]));
    // The start of the next block is the end of this one.
    block_start = block_end;
  }

  accumulate_block<Iterator, T>()(block_start, last, results[num_threads - 1]);

  // https://blog.csdn.net/zhangpiu/article/details/52524614
  // for_each(strs.begin(), strs.end(), mem_fn(&String::half));
  // std::mem_fn()
  std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));

  return std::accumulate(results.begin(), results.end(), init);
}

int main() {
}
#endif