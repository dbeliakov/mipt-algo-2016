#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

template<typename Key, typename Val>
class Map
{
public:
    Val get(const Key& k)
    {
        std::shared_lock<std::shared_mutex> lock(m_);
        //return map_[k]; BAD WAY
        auto it = map_.find(k);
        if (it == map_.end()) {
            throw std::exception(); // TODO
        }
        return it->second;
    }

    void set(const Key& k, const Val& v)
    {
        std::unique_lock<std::shared_mutex> lock(m_);
        map_[k] = v;
    }
private:
    std::unordered_map<Key, Val> map_;
    std::shared_mutex m_;
};

int main()
{
    Map<int, int> map;
    map.set(1, 1);
    std::cout << map.get(1);
    return 0;
}
