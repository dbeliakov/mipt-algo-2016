#include <stack>
#include <thread>

void task(std::stack<int>* s)
{
    for (;;) {
        s->push(1);
        s->pop();
    }
}

int main()
{
    std::stack<int> s;
    std::thread t1(task, &s);
    std::thread t2(task, &s);
    t1.join();
    t2.join();
    return 0;
}

