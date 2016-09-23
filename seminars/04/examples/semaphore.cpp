#include <mutex>
#include <iostream>
#include <thread>

class Semaphore
{
public:
	Semaphore()
		: counter_(0)
	{}

    void post()
    {
    	std::unique_lock<std::mutex> lock(m_);
    	++counter_;
    	cv_.notify_one();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(m_);
    	cv_.wait(lock, [&] { return counter_ > 0; });
    	--counter_;
    }
private:
    std::mutex m_;
    std::condition_variable cv_;
    int counter_;
};

void f(int& x, Semaphore& semaphore)
{
	for (int i = 0; i < 100500; ++i) {
		semaphore.wait();
		++x;
		semaphore.post();
	}
}

int main()
{
	int x = 0;
	Semaphore semaphore;
	semaphore.post();
	std::thread t1(f, std::ref(x), std::ref(semaphore));
	std::thread t2(f, std::ref(x), std::ref(semaphore));
	t1.join();
	t2.join();
	std::cout << x << std::endl;
	return 0;
}
