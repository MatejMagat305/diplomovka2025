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
    const int origin;  // Po�et vl�kien, ktor� musia dorazi� na bari�ru

public:
    explicit MyBarrier(int count) : count(count), origin(count) {}

    void barrier() {
        std::unique_lock<std::mutex> lock(mutex);  // Pou�i unique_lock pre wait()

        if (--count == 0) {
            count = origin;  // Resetujeme count pre �al�ie pou�itie
            cond.notify_all();  // Odomkneme v�etky �akaj�ce vl�kna
        }
        else {
            cond.wait(lock, [this]() { return count == origin; });  // �ak�me na reset countu
        }
    }
};

SYCL_EXTERNAL void processAgentCollisionsGPU(const MemoryPointers& globalMemory, MemoryPointers& localMemory, sycl::nd_item<1> item);
SYCL_EXTERNAL void writeMinimalPath(const MemoryPointers& globalMemory);

void processAgentCollisionsCPUOneThread(const MemoryPointers& globalMemory);
void processAgentCollisionsCPU(const MemoryPointers& globalMemory, MemoryPointers& localMemory, MyBarrier& b, int agentId);
