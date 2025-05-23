#pragma once

#include <cstddef>
#include <vector>
#include <atomic>
#include <span>

class Queue {
public:
~Queue() = default;
Queue(std::size_t number_of_blocks, std::size_t block_size);

Queue(const Queue&) = delete;
Queue(Queue&&) noexcept = delete;

Queue& operator=(const Queue&) = delete;
Queue& operator=(Queue&&) noexcept = delete;

bool popFront(std::span<int> data);
bool pushBack(std::span<int> data);

std::size_t capacity() const;
std::size_t size() const;

bool isFull() const;
bool isEmpty() const;

private:
    //  alignas(64) moves the varibles on to seprate cache line
    alignas(64)  std::atomic_size_t read_index_{0};
    alignas(64)  std::atomic_size_t write_index_{0};
    std::size_t block_size;
    std::vector<int> data_;
};
