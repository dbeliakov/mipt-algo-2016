#pragma once

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
        ++threadsInPop_;
        Node* oldHead = head_.load();
        while (oldHead && !head_.compare_exchange_weak(oldHead, oldHead->next));
        std::shared_ptr<T> res;
        if (oldHead) {
            res.swap(oldHead->data);
        }
        tryReclaim(oldHead);
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

    static void deleteNodes(Node* nodes)
    {
        while (nodes) {
            Node* next = nodes->next;
            delete nodes;
            nodes = next;
        }
    }

    void tryReclaim(Node* oldHead)
    {
        if (threadsInPop_ == 1) {
            Node* nodesToDelete = toBeDeleted_.exchange(nullptr);
            if (!--threadsInPop_) {
                deleteNodes(nodesToDelete);
            } else if (nodesToDelete) {
                chainPendingNodes(nodesToDelete);
            }
            delete oldHead;
        } else {
            chainPendingNode(oldHead);
            --threadsInPop_;
        }
    }

    void chainPendingNodes(Node* nodes)
    {
        Node* last = nodes;
        while (Node* const next = last->next) {
            last = next;
        }
        chainPendingNodes(nodes, last);
    }

    void chainPendingNodes(Node* first, Node* last)
    {
        last->next = toBeDeleted_;
        while (!toBeDeleted_.compare_exchange_weak(last->next, first));
    }

    void chainPendingNode(Node* node) {
        chainPendingNodes(node, node);
    }

    std::atomic<unsigned> threadsInPop_;
    std::atomic<Node*> head_;
    std::atomic<Node*> toBeDeleted_;
};
