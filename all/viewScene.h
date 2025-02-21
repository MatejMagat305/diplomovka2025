#pragma once


#include "scene.h"
#include "map.h"

class ViewScene : public Scene {
private:
    Map* map;
    Scene* StartSimulation();
public:
    ViewScene(Map* map) : map(map) {}
    ~ViewScene();
    Scene* DrawControl() override;
};
