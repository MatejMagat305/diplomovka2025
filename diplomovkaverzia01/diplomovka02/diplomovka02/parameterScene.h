#pragma once

#include "scene.h"

class ParameterScene : public Scene {
    int mapWidth = 1024;
    int mapHeight = 1024;
    int agentCount = 3;
    int obstacleCount = 7;
    int loaderCount = 2;
    int unloaderCount = 2;

    bool activeWidth = false;
    bool activeHeight = false;
    bool activeAgents = false;
    bool activeObstacles = false;
    bool activeMarkers = false;
    bool activeLoaders = false;
    bool activeUnloaders = false;
    bool alert = false;
public:
    ParameterScene();
    Scene* DrawControl() override;
    ~ParameterScene() = default;
};
