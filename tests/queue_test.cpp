/*
 * This file is part of lock_free.
 *
 * lock_free is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * lock_free is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with lock_free.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <catch2/catch_test_macros.hpp>

#include "../src/queue.hpp"

#include <atomic>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <thread>
#include <vector>

TEST_CASE("Factorials are computed", "[queue]") {
  constexpr auto block_size = 4;
  constexpr auto num_blocks = 8;
  Queue<int> queue(num_blocks, block_size);

  std::vector<int> input_block = {11, 21, 31, 41};
  std::vector<int> output_block(block_size);

  SECTION("empty") {
    REQUIRE(queue.isEmpty());
    REQUIRE_FALSE(queue.isFull());
  }

  SECTION("Push and Pop") {
    REQUIRE(queue.pushBack(input_block));
    REQUIRE_FALSE(queue.isEmpty());

    REQUIRE(queue.popFront(output_block));
    REQUIRE(queue.isEmpty());
    REQUIRE(output_block == input_block);
  }

  SECTION("Fill the Q") {
    std::vector<int> fill_test_block = {-1, 0, 0, 0};

    for (int i = 0; i < queue.getCapacity(); ++i) {
      fill_test_block[0] = i;
      REQUIRE(queue.pushBack(fill_test_block));
    }

    REQUIRE(queue.isFull());

    for (int i = 0; i < queue.getCapacity(); ++i) {
      REQUIRE(queue.popFront(output_block));
      REQUIRE(output_block[0] == i);
    }
    REQUIRE(queue.isEmpty());
  }

  SECTION("Overfill") {
    for (int i = 0; i < queue.getCapacity(); ++i) {
      REQUIRE(queue.pushBack(input_block));
    }
    REQUIRE_FALSE(queue.pushBack(input_block));
    REQUIRE(queue.popFront(output_block));
  }
}

// TEST_CASE("Manual throughput measurement", "[throughput]") {
//   constexpr std::size_t block_size = 8;
//   constexpr std::size_t num_blocks = 1024;
//   Queue q(num_blocks, block_size);
//   std::vector<int> block(block_size, 1);

//   constexpr int iterations = 1'000'000;
//   auto start = std::chrono::high_resolution_clock::now();

//   for (int i = 0; i < iterations; ++i) {
//     while (!q.pushBack(block)) {
//     }
//   }

//   auto end = std::chrono::high_resolution_clock::now();
//   double seconds = std::chrono::duration<double>(end - start).count();
//   double throughput = iterations / seconds;

//   INFO("Throughput: " << throughput << " ops/sec\n");
// }

TEST_CASE("Queue multithreaded use", "[Queue][threads]") {
  constexpr std::size_t block_size = 8;
  constexpr std::size_t num_blocks = 64;
  Queue<int> queue(num_blocks, block_size);

  constexpr int num_iterations = 1000;

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
        REQUIRE(std::all_of(data.begin(), data.end(),
                            [](int v) { return v == 42; }));
        ++count;
      } else {
        if (producer_done.load())
          break;
        std::this_thread::yield();
      }
    }
    REQUIRE(count == num_iterations);
  });

  producer.join();
  consumer.join();
}