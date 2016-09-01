#include <iostream>
#include <thread>
#include <atomic>
#include <vector>

class LamportMutex
{
public:
    LamportMutex(int n) : want_(n), ticket_(n) {
        init(n);
    }

    void lock(int t) {
        chooseTicket(t);
        waitInQueue(t);
    }

    void unlock(int t) {
        want_[t].store(false);
    }

private:
    // doorway section, finite number of steps
    void chooseTicket(int t) {
        want_[t].store(true);;
        int maxTicket = 0;
        for (int i = 0; i < ticket_.size(); ++i) {
            maxTicket = std::max(maxTicket, ticket_[i].load());
        }
        ticket_[t].store(maxTicket + 1);
    }

    // wait section
    void waitInQueue(int t) {
        for (int i = 0; i < ticket_.size(); ++i) {
            while (want_[i] && std::make_pair(ticket_[i].load(), i) < std::make_pair(ticket_[t].load(), t)) {
                /* wait */
            }
        }
    }

    void init(int n) {
        for (size_t i = 0; i < n; ++i) {
            want_[i].store(false);
            ticket_[i].store(0);
        }
    }

    std::vector<std::atomic<bool>> want_;
    std::vector<std::atomic<int>> ticket_;
};

void increment(int* x, LamportMutex* m, int thread)
{
   for (size_t i = 0; i < 10000; ++i) {
       m->lock(thread);
       ++*x;
       m->unlock(thread);
   }
}

int main()
{
    LamportMutex m(2);
    int x = 0;
    std::thread t1(increment, &x, &m, 0);
    std::thread t2(increment, &x, &m, 1);
    t1.join();
    t2.join();
    std::cout << x << std::endl;
    return 0;
}

