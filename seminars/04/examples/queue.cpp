#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <chrono>

class InterruptException
{};

template<typename T, size_t Max>
class Queue
{
public:
    void push(const T& obj)
    {
        std::unique_lock<std::mutex> uLock(mutex_);
        cvPush_.wait(uLock, [&]() { return (queue_.size() <= Max) || finished_;});
        std::cout << "push :: Queue.size = " << queue_.size() << std::endl;
        if (queue_.size() > Max) {
            throw InterruptException();
        }
        queue_.push(obj);
        cv_.notify_one();
    }

    T pop()
    {
        std::unique_lock<std::mutex> uLock(mutex_);
        cv_.wait(uLock, [&]() { return !queue_.empty() || finished_; });
        if (queue_.empty()) {
            throw InterruptException();
        }
        auto obj = queue_.front();
        queue_.pop();
        cvPush_.notify_one();
        return obj;
    }

    void finish()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        finished_ = true;
        cv_.notify_all();
        cvPush_.notify_all();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable cvPush_;
    bool finished_ = false;
};

void push(Queue<std::string, 42>& queue)
{
    for (size_t i = 0; i < 100500; ++i) {
        queue.push("NUM: " + std::to_string(i));
    }
}

void print(Queue<std::string, 42>& queue)
{
    try {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        for (;;) {
            std::cout << queue.pop() << std::endl;
        }
    } catch (InterruptException&) {
        std::cout << "Finished" << std::endl;
    }
}

int main()
{
    const int maxQueSize = 42;
    Queue<std::string, maxQueSize> q;
    auto printer = std::thread(print, std::ref(q));
    auto pusher1 = std::thread(push, std::ref(q));
    auto pusher2 = std::thread(push, std::ref(q));
    auto pusher3 = std::thread(push, std::ref(q));
    pusher1.join();
    pusher2.join();
    pusher3.join();
    q.finish();
    printer.join();
    return 0;
}

