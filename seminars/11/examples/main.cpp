#include "lock_free_stack_hp.h"
#include <iostream>

int main()
{
    LockFreeStack<int> s;
    s.push(1);
    std::cout << *s.pop();
    return 0;
}
