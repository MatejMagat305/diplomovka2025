#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "map.h"

class Simulation {
public:
    static std::atomic<bool> runAble; // Flag na ukon�enie simul�cie

    explicit Simulation(Map& map) : map(map) {}

    void start();



private:
    Map& map;
    std::vector<std::thread> agentThreads;
    std::mutex syncMutex;
    std::condition_variable syncCondition;
    bool replanFlag = false; // Indikuje, �i niektor� agent prepl�noval cestu

    void controlThread();
};