#include <mutex>
#include <iostream>

int main()
{
    std::recursive_mutex m;
    m.lock();
    std::cout << "1 lock" << std::endl;
    m.lock();
    std::cout << "2 lock" << std::endl;
    m.unlock();
    m.unlock();
    return 0;
}
