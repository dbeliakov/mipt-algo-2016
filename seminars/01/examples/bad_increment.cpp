#include <iostream>
#include <thread>

void increment(int* x)
{
    for (int i = 0; i < 10000; ++i)
    {
        ++*x;
    }
}

int main()
{
    int x = 0;
    std::thread t1(increment, &x);
    std::thread t2(increment, &x);
    t1.join();
    t2.join();
    std::cout << x << std::endl;
    return 0;
}

