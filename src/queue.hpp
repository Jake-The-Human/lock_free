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

#pragma once

#include <atomic>
#include <cstddef>
#include <span>
#include <vector>

class Queue {
public:
  ~Queue() = default;
  Queue(std::size_t number_of_blocks, std::size_t block_size);

  Queue(const Queue &) = delete;
  Queue(Queue &&) noexcept = delete;

  Queue &operator=(const Queue &) = delete;
  Queue &operator=(Queue &&) noexcept = delete;

  bool popFront(std::span<int> data);
  bool pushBack(std::span<int> data);

  bool isFull() const;
  bool isEmpty() const;

private:
  //  alignas(64) moves the varibles on to seprate cache line
  alignas(64) std::atomic_size_t read_index_{0};
  alignas(64) std::atomic_size_t write_index_{0};
  alignas(64) std::atomic_size_t size_{0};
  std::size_t block_size_;
  std::size_t capacity_;
  std::vector<int> data_;
};
