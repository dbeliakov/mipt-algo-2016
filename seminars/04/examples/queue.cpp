#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <chrono>

class InterruptException
{};

template<typename T>
class Queue
{
public:
    void push(const T& obj)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(obj);
        cv_.notify_one();
    }

    T pop()
    {
        std::unique_lock<std::mutex> uLock(mutex_);
        cv_.wait(uLock, [&] { return !queue_.empty() || finished_; });
        if (queue_.empty()) {
            throw InterruptException();
        }
        auto obj = queue_.front();
        queue_.pop();
        return obj;
    }

    void finish()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        finished_ = true;
        cv_.notify_one();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool finished_ = false;
};

void push(Queue<std::string>& queue)
{
    for (size_t i = 0; i < 5; ++i) {
        queue.push("NUM: " + std::to_string(i));
    }
}

void print(Queue<std::string>& queue)
{
    try {
        for (;;) {
            std::cout << queue.pop() << std::endl;
        }
    } catch (InterruptException&) {
        std::cout << "Finished" << std::endl;
    }
}

int main()
{
    Queue<std::string> q;
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

