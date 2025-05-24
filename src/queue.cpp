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

#include "queue.hpp"
#include <atomic>
#include <cassert>
#include <immintrin.h>

constexpr auto MAX_RETRIES = 100;

Queue::Queue(std::size_t number_of_blocks, std::size_t block_size)
    : block_size_(block_size), capacity_(number_of_blocks),
      data_(number_of_blocks * block_size) {
  assert(data_.size() % block_size == 0);
}

bool Queue::popFront(std::span<int> out_data) {
  if (out_data.size() != block_size_)
    return false;

  for (int i = 0; i < MAX_RETRIES; ++i) {
    auto current_read_index = read_index_.load(std::memory_order_acquire);
    auto next_read_index = (current_read_index + block_size_) % data_.size();

    if (size_.load(std::memory_order_acquire) == 0) {
      break;
    }

    if (read_index_.compare_exchange_weak(current_read_index, next_read_index,
                                          std::memory_order_acq_rel,
                                          std::memory_order_relaxed)) {
      auto end = current_read_index + block_size_;
      std::copy(data_.begin() + current_read_index, data_.begin() + end,
                out_data.begin());
      size_.fetch_sub(1, std::memory_order_acq_rel);
      return true;
    }
    _mm_pause();
  }
  return false;
}

bool Queue::pushBack(std::span<int> in_data) {
  if (in_data.size() != block_size_)
    return false;

  for (int i = 0; i < MAX_RETRIES; ++i) {
    auto current_write_index = write_index_.load(std::memory_order_acquire);
    auto next_write_index = (current_write_index + block_size_) % data_.size();

    if (size_.load(std::memory_order_acquire) == capacity_) {
      break;
    }

    if (write_index_.compare_exchange_weak(
            current_write_index, next_write_index, std::memory_order_acq_rel,
            std::memory_order_relaxed)) {
      std::copy(in_data.begin(), in_data.end(),
                data_.begin() + current_write_index);
      size_.fetch_add(1, std::memory_order_acq_rel);
      return true;
    }
    _mm_pause();
  }
  return false;
}

bool Queue::isFull() const {
  return size_.load(std::memory_order_acquire) == capacity_;
}

bool Queue::isEmpty() const {
  return size_.load(std::memory_order_acquire) == 0;
}
