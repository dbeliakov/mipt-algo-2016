#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

class TicketSpinlock
{
    using ticket = std::size_t;

public:
    TicketSpinlock()
        : ownerTicket_(0)
        , nextFreeTicket_(0)
    {}

    void lock()
    {
        ticket thisThreadTicket = nextFreeTicket_.fetch_add(1);
        while (ownerTicket_.load() != thisThreadTicket) {
            // wait
        }
    }

    void unlock() {
        ownerTicket_.store(ownerTicket_.load() + 1);
    }

private:
    std::atomic<ticket> ownerTicket_;
    std::atomic<ticket> nextFreeTicket_;
};

void increment(int& x, TicketSpinlock& m)
{
    for (size_t i = 0; i < 10000; ++i) {
        m.lock();
        ++x;
        m.unlock();
    }
}

int main()
{
    TicketSpinlock m;
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

