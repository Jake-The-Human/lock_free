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

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <immintrin.h>
#include <span>
#include <vector>

namespace {
constexpr auto MAX_RETRIES = 100;
} // namespace

/**
 * @class Queue
 * @brief A lock-free fixed-capacity circular buffer for blocks of elements of
 * type T.
 *
 * This class implements a single-producer, single-consumer
 * lock-free queue where each element is a fixed-size block of type `T`.
 * It uses atomic operations and cache line alignment to reduce contention.
 *
 * @tparam T The type of element stored in each block.
 */
template <typename T> class Queue {
public:
  /**
   * @brief Constructs a new Queue.
   *
   * @param number_of_blocks Number of blocks to allocate (must be a power of
   * two).
   * @param block_size Number of `T` elements in each block.
   *
   * The effective usable capacity is `number_of_blocks - 1` to distinguish
   * full vs empty state unambiguously.
   */
  Queue(std::size_t number_of_blocks, std::size_t block_size)
      : block_size_(block_size), data_(number_of_blocks * block_size) {
    assert((block_size_ & (block_size_ - 1)) == 0 &&
           "block_size must be a power of two");
    assert((number_of_blocks & (number_of_blocks - 1)) == 0 &&
           "number_of_blocks must be a power of two");
    static_assert(sizeof(PaddedIndex) == 64,
                  "PaddedIndex must occupy exactly one cache line");
  }

  /// @brief Default destructor.
  ~Queue() = default;

  /// @brief Deleted copy constructor.
  Queue(const Queue &) = delete;

  /// @brief Deleted move constructor.
  Queue(Queue &&) noexcept = delete;

  /// @brief Deleted copy assignment.
  Queue &operator=(const Queue &) = delete;

  /// @brief Deleted move assignment.
  Queue &operator=(Queue &&) noexcept = delete;

  /**
   * @brief Pops a block from the front of the queue.
   *
   * @param data Output span to receive the popped block. Must match block size.
   * @return true if a block was successfully popped.
   * @return false if the queue was empty or the span size is invalid.
   *
   * This operation is lock-free and thread-safe.
   */
  [[nodiscard]] bool popFront(std::span<T> out_data) {
    if (out_data.size() != block_size_)
      return false;

    for (int i = 0; i < MAX_RETRIES; ++i) {
      auto current_read_index =
          read_index_.value.load(std::memory_order_relaxed);
      auto next_read_index =
          (current_read_index + block_size_) & (data_.size() - 1);

      if (current_read_index ==
          write_index_.value.load(std::memory_order_relaxed)) {
        break; // empty
      }

      if (read_index_.value.compare_exchange_weak(
              current_read_index, next_read_index, std::memory_order_acq_rel,
              std::memory_order_relaxed)) {
        std::copy_n(&data_[current_read_index], block_size_, out_data.data());
        return true;
      }

      _mm_pause(); // _mm_pause stops popFront from hogging the Queue
    }

    return false;
  }

  /**
   * @brief Pushes a block to the back of the queue.
   *
   * @param data Input span containing data to enqueue. Must match block size.
   * @return true if the block was successfully enqueued.
   * @return false if the queue was full or the span size is invalid.
   *
   * This operation is lock-free and thread-safe.
   */
  [[nodiscard]] bool pushBack(std::span<T> in_data) {
    if (in_data.size() != block_size_)
      return false;

    for (int i = 0; i < MAX_RETRIES; ++i) {
      auto current_write_index =
          write_index_.value.load(std::memory_order_relaxed);
      auto next_write_index =
          (current_write_index + block_size_) & (data_.size() - 1);

      if (next_write_index ==
          read_index_.value.load(std::memory_order_relaxed)) {
        break; // full
      }

      if (write_index_.value.compare_exchange_weak(
              current_write_index, next_write_index, std::memory_order_acq_rel,
              std::memory_order_relaxed)) {
        std::copy_n(in_data.data(), block_size_, &data_[current_write_index]);
        return true;
      }

      _mm_pause(); // _mm_pause stops popFront from hogging the Queue
    }

    return false;
  }

  /**
   * @brief Checks whether the queue is full.
   *
   * @return true if the queue is full and cannot accept more data.
   */
  [[nodiscard]] bool isFull() const {
    auto next =
        (write_index_.value.load(std::memory_order_relaxed) + block_size_) &
        (data_.size() - 1);
    return next == read_index_.value.load(std::memory_order_relaxed);
  }

  /**
   * @brief Checks whether the queue is empty.
   *
   * @return true if the queue is empty and no data is available.
   */
  [[nodiscard]] bool isEmpty() const {
    return read_index_.value.load(std::memory_order_relaxed) ==
           write_index_.value.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns the usable block capacity of the queue.
   *
   * @return The maximum number of blocks that can be enqueued.
   *
   * One slot is reserved to distinguish full from empty state,
   * so the capacity is `number_of_blocks - 1`.
   */
  [[nodiscard]] std::size_t getCapacity() const {
    return (data_.size() / block_size_) - 1;
  }

  /// @brief Clears the queue by resetting the read and write indices.
  void clear() {
    read_index_.value.store(0, std::memory_order_release);
    write_index_.value.store(0, std::memory_order_release);
  }

private:
  /**
   * @brief Struct to store an atomic index aligned to a cache line boundary.
   *
   * This struct is used to prevent **false sharing** between the `read_index_`
   * and `write_index_` in the `Queue` class. False sharing occurs when two
   * threads on different cores modify variables that reside on the same cache
   * line, causing unnecessary cache coherency traffic and performance
   * degradation.
   *
   * By aligning each `PaddedIndex` instance to a 64-byte cache line (common
   * size on x86), and padding the structure to occupy a full cache line, we
   * ensure that the `value` member resides on its own cache line and does not
   * interfere with neighboring atomic variables.
   */
  struct alignas(64) PaddedIndex {
    /// @brief The atomic index value.
    std::atomic_size_t value{0};

    /// @brief Padding to fill out a full 64-byte cache line.
    /// Ensures that no other variable shares the same cache line.
    char padding[64 - sizeof(std::atomic_size_t)];
  };

  std::size_t block_size_; ///< Size of each block in number of `T`s.
  std::vector<T> data_;    ///< Internal circular buffer storing all blocks.

  /// @brief Index for the next block to read from.
  /// Aligned to cache line to minimize false sharing.
  PaddedIndex read_index_{0};

  /// @brief Index for the next block to write to.
  /// Aligned to cache line to minimize false sharing.
  PaddedIndex write_index_{0};
};
