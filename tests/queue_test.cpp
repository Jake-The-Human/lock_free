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

TEST_CASE("Factorials are computed", "[queue]") {
  constexpr auto block_size = 4;
  constexpr auto num_blocks = 8;
  Queue queue(num_blocks, block_size);

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

    for (int i = 0; i < num_blocks; ++i) {
      fill_test_block[0] = i;
      REQUIRE(queue.pushBack(fill_test_block));
    }

    REQUIRE(queue.isFull());

    for (int i = 0; i < num_blocks; ++i) {
      REQUIRE(queue.popFront(output_block));
      REQUIRE(output_block[0] == i);
    }
    REQUIRE(queue.isEmpty());
  }

  SECTION("Overfill") {
    for (int i = 0; i < num_blocks; ++i) {
      REQUIRE(queue.pushBack(input_block));
    }
    REQUIRE_FALSE(queue.pushBack(input_block));
    REQUIRE(queue.popFront(output_block));
  }
}

TEST_CASE("Wrap around", "[Queue]") {}

TEST_CASE("multithreaded"
          "[Queue][threads]") {}
