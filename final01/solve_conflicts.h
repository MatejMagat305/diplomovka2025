#pragma once
#include "heap_primitive.h"

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
    const int origin;
    int cycle = 0;

public:
    explicit MyBarrier(int count) : count(count), origin(count) {}

    void barrier() {
        std::unique_lock<std::mutex> lock(mutex);
        int local_cycle = cycle;
        if (--count == 0) {
            count = origin;
            cycle++;
            cond.notify_all();
        }
        else {
            cond.wait(lock, [this, local_cycle]() { return local_cycle != cycle; });
        }
    }
};

SYCL_EXTERNAL void processAgentCollisionsGPU(const MemoryPointers& globalMemory, MemoryPointers& localMemory, sycl::nd_item<1> item);
SYCL_EXTERNAL void writeMinimalPath(const MemoryPointers& globalMemory);
SYCL_EXTERNAL bool shouldYield(int agentId, int conflictAgentId, Agent* agents);

void processAgentCollisionsCPUOneThread(const MemoryPointers& globalMemory);
void processAgentCollisionsCPU(const MemoryPointers& globalMemory, MemoryPointers& localMemory, MyBarrier& b, int agentId);
