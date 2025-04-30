#pragma once
#include "scene.h"
#include "map.h"
#include "memSimulation.h"
#include <mutex>



class SimulationScene : public Scene {
private:
    void switchComputeType();
    void runSimulation();
    MemSimulation* mem;
    std::mutex mtx;
    bool shouldEnd = false;

public:
    SimulationScene(Map* map);
    SimulationScene(MemSimulation *mem);
    Scene* DrawControl() override;
    ~SimulationScene();
    void tik();
};