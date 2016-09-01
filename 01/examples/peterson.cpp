#include <iostream>
#include <thread>
#include <atomic>
#include <array>

class PetersonMutex
{
public:
    PetersonMutex()
    {
        want_[0].store(false);
        want_[1].store(false);
        victim_.store(0);
    }

    void lock(int t)
    {
        want_[t].store(true);
        victim_.store(t);
        while (want_[1 - t].load() && victim_.load() == t) {
            // wait
        }
    }

    void unlock(int t) {
        want_[t].store(false);
    }

private:
    std::array<std::atomic<bool>, 2> want_;
    std::atomic<int> victim_;
};

void increment(int* x, PetersonMutex* m, int thread)
{
   for (size_t i = 0; i < 10000; ++i) {
       m->lock(thread);
       ++*x;
       m->unlock(thread);
   }
}

int main()
{
    PetersonMutex m;
    int x = 0;
    std::thread t1(increment, &x, &m, 0);
    std::thread t2(increment, &x, &m, 1);
    t1.join();
    t2.join();
    std::cout << x << std::endl;
    return 0;
}

