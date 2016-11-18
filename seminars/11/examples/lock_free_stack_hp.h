#pragma once

#include "hazard_pointer.h"

#include <atomic>
#include <memory>

template<typename T>
class LockFreeStack
{
public:
    void push(const T& data)
    {
        Node* const newNode = new Node(data);
        newNode->next = head_.load();
        while (!head_.compare_exchange_weak(newNode->next, newNode));
    }

    std::shared_ptr<T> pop()
    {
        std::atomic<void*>& hp = getHazardPointerForCurrentThread();
        Node* oldHead = head_.load();
        do {
            Node* tmp = nullptr;
            do {
                tmp = oldHead;
                hp.store(oldHead);
                oldHead = head_.load();
            } while (oldHead != tmp);
        } while (oldHead && !head_.compare_exchange_strong(oldHead, oldHead->next));
        hp.store(nullptr);
        std::shared_ptr<T> res;
        if (oldHead) {
            res.swap(oldHead->data);
            if (outstandingHazardPointersFor(oldHead)) {
                reclaimLater(oldHead);
            } else {
                delete oldHead;
            }
            deleteNodesWithNoHazards();
        }
        return res;
    }

private:
    struct Node
    {
        Node(const T& data_)
            : data(std::make_shared<T>(data_))
        {}

        std::shared_ptr<T> data;
        Node* next;
    };

    std::atomic<Node*> head_;
};
