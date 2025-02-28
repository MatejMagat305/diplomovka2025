#pragma once
#include "scene.h"
#include "map.h"
#include <mutex>
#include <thread>
#include <atomic>
#include "device_type_algoritmus.h"
#include <map>
#include <vector>

class SimulationScene : public Scene {
private:
    int indexType;
    std::string buttonMsg;
    std::map<ComputeType, std::string> stringMap{};
    std::vector<ComputeType> method{};
    bool hasInfo = false;
    Info i;
    Map* map;
    std::mutex simMutex;
    std::thread simThread;
    std::atomic<bool> isRunning = false;
    void switchComputeType();
    void runSimulation();


public:
    SimulationScene(Map* map);
    Scene* DrawControl() override;
    ~SimulationScene();
};