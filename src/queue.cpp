#include "queue.hpp"
#include <atomic>
#include <cassert>
#include <immintrin.h>

constexpr auto MAX_RETRIES = 100;

Queue::Queue(std::size_t number_of_blocks, std::size_t block_size)
    : block_size(block_size), data_(number_of_blocks * block_size) {
  assert(data_.size() % block_size == 0);
}

bool Queue::popFront(std::span<int> out_data) {
  if (out_data.size() != block_size)
    return false;

  for (int i = 0; i < MAX_RETRIES; ++i) {
    auto current_read_index = read_index_.load(std::memory_order_acquire);
    auto next_read_index = (current_read_index + block_size) % data_.size();

    auto current_write_index = write_index_.load(std::memory_order_acquire);
    if (current_read_index == current_write_index) {
      break;
    }

    if (read_index_.compare_exchange_weak(current_read_index, next_read_index,
                                          std::memory_order_acq_rel,
                                          std::memory_order_relaxed)) {
      auto end = current_read_index + block_size;
      std::copy(data_.begin() + current_read_index, data_.begin() + end,
                out_data.begin());
      return true;
    }
    _mm_pause();
  }
  return false;
}

bool Queue::pushBack(std::span<int> in_data) {
  if (in_data.size() != block_size)
    return false;

  for (int i = 0; i < MAX_RETRIES; ++i) {
    auto current_write_index = write_index_.load(std::memory_order_acquire);
    auto next_write_index = (current_write_index + block_size) % data_.size();

    auto current_read_index = read_index_.load(std::memory_order_acquire);
    if (next_write_index == current_read_index) {
      break;
    }

    if (write_index_.compare_exchange_weak(
            current_write_index, next_write_index, std::memory_order_acq_rel,
            std::memory_order_relaxed)) {
      std::copy(in_data.begin(), in_data.end(),
                data_.begin() + current_write_index);
      return true;
    }
    _mm_pause();
  }
  return false;
}

bool Queue::isFull() const {
  auto current_write_index = write_index_.load(std::memory_order_acquire);
  auto next_write_index = (current_write_index + block_size) % data_.size();
  return next_write_index == read_index_.load(std::memory_order_acquire);
}
bool Queue::isEmpty() const {
  return read_index_.load(std::memory_order_acquire) ==
         write_index_.load(std::memory_order_acquire);
}
