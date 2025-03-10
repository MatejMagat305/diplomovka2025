#pragma once

#include <mutex>
#include <thread>
#include <atomic>
#include "device_type_algoritmus.h"
#include <map>
#include <vector>
#include "map.h"

struct MemSimulation {
    int indexType;
    std::string buttonMsg;
    std::map<ComputeType, std::string> stringMap{};
    std::vector<ComputeType> method{};
    bool hasInfo = false, isCPUOverGPU = false;
    Info i;
    std::mutex simMutex;
    std::thread simThread;
    std::atomic<bool> isRunning = false;
    Map* map;

    ~MemSimulation() {
      if (simThread.joinable()) {
          simThread.join();
      }
      if (map != nullptr) {
          delete map;
      }
    }
};