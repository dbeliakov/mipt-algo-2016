#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

class Spinlock
{
public:
    Spinlock()
        : locked_(false)
    {}

    void lock()
    {
        while (locked_.exchange(true)) {
            // wait
        }
    }

    void unlock() {
        locked_.store(false);
    }

private:
    std::atomic<bool> locked_;
};

void increment(int& x, Spinlock& m)
{
    for (size_t i = 0; i < 10000; ++i) {
        m.lock();
        ++x;
        m.unlock();
    }
}

int main()
{
    Spinlock m;
    int x;
    std::vector<std::thread> threads;
    for (size_t i = 0; i < 3; ++i) {
        threads.emplace_back(increment, std::ref(x), std::ref(m));
    }
    for (auto& t : threads) {
        t.join();
    }
    std::cout << x << std::endl;
    return 0;
}

