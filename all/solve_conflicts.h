#pragma once
#include "heap_primitive.h"
#include "a_star_algo.h"

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <sycl/sycl.hpp>

namespace internal {
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

    void processAgentCollisionsGPU(sycl::nd_item<1> item, Position* paths, int* pathSizes,
        int* width_height_loaderCount_unloaderCount_agentCount, int* minSize, char* grid,
        Constrait* constrait, int* numberConstrait, Agent* agents);
    void processAgentCollisionsCPU(MyBarrier& b, int agentId, Position* paths, int* pathSizes,
        int* width_height_loaderCount_unloaderCount_agentCount, int* minSize, char* grid,
        Constrait* constrait, int* numberConstrait, Agent* agents);
}