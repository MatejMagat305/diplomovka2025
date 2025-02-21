#pragma once
#include "scene.h"
#include "map.h"

class SimulationScene : public Scene {
private:
    Map* map;

    void simulateStep();


public:
    SimulationScene(Map* map) : map(map) {}
    Scene* DrawControl() override;
    ~SimulationScene();
};