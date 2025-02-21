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

    void processAgentCollisionsGPU(sycl::nd_item<1> item, Position* paths, int* pathSizes,
        int* width_height_loaderCount_unloaderCount_agentCount, int* minSize, char* grid,
        Constrait* constrait, int* numberConstrait, Agent* agents);
    void processAgentCollisionsCPU(MyBarrier& b, int agentId, Position* paths, int* pathSizes,
        int* width_height_loaderCount_unloaderCount_agentCount, int* minSize, char* grid,
        Constrait* constrait, int* numberConstrait, Agent* agents);
}