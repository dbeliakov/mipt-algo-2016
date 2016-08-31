#include <iostream>
#include <thread>

int main()
{
    std::thread th([] {std::cout << "Hello, world!" << std::endl;});
    th.join();
    return 0;
}
