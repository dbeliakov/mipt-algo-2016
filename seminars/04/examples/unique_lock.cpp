#include <iostream>
#include <mutex>
#include <chrono>

int main()
{
    std::timed_mutex m;
    // Try lock for 2 seconds
    std::unique_lock<std::timed_mutex> lock(m, std::chrono::seconds(2));
    // if owns mutex, unlock it
    if (!!lock) {
        lock.unlock();
    }
    return 0;
}
