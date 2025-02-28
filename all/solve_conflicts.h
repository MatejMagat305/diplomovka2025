#pragma once
#include "heap_primitive.h"
#include "a_star_algo.h"

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <sycl/sycl.hpp>
#include "map.h"

class MyBarrier {
    std::mutex mutex;
    std::condition_variable cond;
    int count;
    const int origin;  // PoËet vl·kien, ktorÈ musia doraziù na bariÈru

public:
    explicit MyBarrier(int count) : count(count), origin(count) {}

    void barrier() {
        std::unique_lock<std::mutex> lock(mutex);  // Pouûi unique_lock pre wait()

        if (--count == 0) {
            count = origin;  // Resetujeme count pre Ôalöie pouûitie
            cond.notify_all();  // Odomkneme vöetky Ëakaj˙ce vl·kna
        }
        else {
            cond.wait(lock, [this]() { return count == origin; });  // »ak·me na reset countu
        }
    }
};

SYCL_EXTERNAL void processAgentCollisionsGPU(sycl::nd_item<1> item, MemoryPointers& localMemory, const MemoryPointers& globalMemory);
void processAgentCollisionsCPU(MyBarrier& b, int agentId, MemoryPointers& localMemory, const MemoryPointers& globalMemory);
SYCL_EXTERNAL void writeMinimalPath(const MemoryPointers& globalMemory);