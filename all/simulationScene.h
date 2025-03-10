#pragma once
#include "scene.h"
#include "map.h"
#include "memSimulation.h"

class SimulationScene : public Scene {
private:
    void switchComputeType();
    void runSimulation();
    MemSimulation* mem;

public:
    SimulationScene(Map* map);
    SimulationScene(MemSimulation *mem);
    Scene* DrawControl() override;
    ~SimulationScene();
};