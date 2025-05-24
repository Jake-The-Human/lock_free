#include "../src/queue.hpp"
#include <atomic>
#include <barrier>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  constexpr std::size_t block_size = 64;
  constexpr std::size_t num_blocks = 1024;
  constexpr int num_iterations = 1'000'000;

  std::vector<int> data(block_size, 42);
  std::vector<int> out(block_size);
  std::barrier sync_point(2); // 1 producer + 1 consumer

  Queue<int> queue(num_blocks, block_size);

  auto start = std::chrono::steady_clock::now();

  std::atomic<bool> producer_done = false;
  std::thread producer([&] {
    std::vector<int> data(block_size, 42);
    for (int i = 0; i < num_iterations; ++i) {
      while (!queue.pushBack(data)) {
        std::this_thread::yield();
      }
    }
    producer_done = true;
  });

  std::thread consumer([&] {
    std::vector<int> data(block_size);
    int count = 0;
    while (count < num_iterations) {
      if (queue.popFront(data)) {
        ++count;
      } else {
        if (producer_done.load())
          break;
        std::this_thread::yield();
      }
    }
  });

  producer.join();
  consumer.join();

  auto end = std::chrono::steady_clock::now();
  auto duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  std::cout << "Total time: " << duration_us << " us\n";
  std::cout << "Throughput: " << (num_iterations * 1'000'000.0 / duration_us)
            << " blocks/sec\n";

  return 0;
}
