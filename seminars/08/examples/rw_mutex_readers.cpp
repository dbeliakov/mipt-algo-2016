#include <mutex>

class RWMutex
{
public:
    void write_lock()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&] { return readers_ == 0 && !writing_; });
        writing_ = true;
    }

    void write_unlock()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        writing_ = false;
        cv_.notify_all();
    }

    void read_lock()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&] { return !writing_; });
        ++readers_;
    }

    void read_unlock()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        --readers_;
        if (readers_ == 0) {
            cv_.notify_one();
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool writing_;
    size_t readers_;
};

int main()
{
    return 0;
}
