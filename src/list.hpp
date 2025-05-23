#pragma once

#include <cstddef>
#include <atomic>

struct Node {
    int data = 0;
    std::atomic<Node*> prev{nullptr};
    std::atomic<Node*> next{nullptr};
};

class LinkList {
public:
    ~LinkList();
    LinkList();
    LinkList(const LinkList&) = delete;
    LinkList(LinkList&&) = delete;
    LinkList& operator=(const LinkList&) = delete;
    LinkList& operator=(LinkList&&) = delete;

    void addTail();
    void removeTail();
    void addFront();
    void removeFront();

    std::size_t size() const;

private:
    std::atomic<Node*> head_{nullptr};
    std::atomic<Node*> tail_{nullptr};
    std::atomic_size_t size_{0};
};