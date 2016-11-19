#pragma once

#include <atomic>
#include <thread>

unsigned const MAX_HAZARD_POINTERS = 100;

struct HazardPointer
{
    std::atomic<std::thread::id> id;
    std::atomic<void*> pointer;
};
HazardPointer g_hazardPointers[MAX_HAZARD_POINTERS];

class HPOwner
{
public:
    HPOwner(const HPOwner&) = delete;
    HPOwner operator=(const HPOwner&) = delete;

    HPOwner()
        : hp_(nullptr)
    {
        for (unsigned i = 0; i < MAX_HAZARD_POINTERS; ++i) {
            std::thread::id oldId;
            if (g_hazardPointers[i].id.compare_exchange_strong(
                    oldId, std::this_thread::get_id())) {
                hp_ = &g_hazardPointers[i];
                break;
            }
        }
        if (!hp_) {
            throw std::runtime_error("No hazard pointers available");
        }
    }

    std::atomic<void*>& getPointer()
    {
        return hp_->pointer;
    }

    ~HPOwner()
    {
        hp_->pointer.store(nullptr);
        hp_->id.store(std::thread::id());
    }

private:
    HazardPointer* hp_;
};

std::atomic<void*>& getHazardPointerForCurrentThread()
{
    thread_local static HPOwner hazard;
    return hazard.getPointer();
}

bool outstandingHazardPointersFor(void* p)
{
    for (unsigned i = 0; i < MAX_HAZARD_POINTERS; ++i) {
        if (g_hazardPointers[i].pointer.load() == p) {
            return true;
        }
    }
    return false;
}

template<typename T>
void doDelete(void* p)
{
    delete static_cast<T*>(p);
}

struct DataToReclaim
{
    void* data;
    std::function<void(void*)> deleter;
    DataToReclaim* next;

    template<typename T>
    DataToReclaim(T* p)
        : data(p)
        , deleter(&doDelete<T>)
        , next(nullptr)
    {}

    ~DataToReclaim()
    {
        deleter(data);
    }
};

std::atomic<DataToReclaim*> nodesToReclaim;

void addToReclaimList(DataToReclaim* node)
{
    node->next = nodesToReclaim.load();
    while (!nodesToReclaim.compare_exchange_weak(node->next, node));
}

template<typename T>
void reclaimLater(T* data)
{
    addToReclaimList(new DataToReclaim(data));
}

void deleteNodesWithNoHazards()
{
    DataToReclaim* current = nodesToReclaim.exchange(nullptr);
    while (current) {
        DataToReclaim* const next = current->next;
        if (!outstandingHazardPointersFor(current->data)) {
            delete current;
        } else {
            addToReclaimList(current);
        }
        current = next;
    }
}










