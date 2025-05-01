#pragma once


#include "scene.h"
#include "map.h"

class ViewScene : public Scene {
private:
    Map* map;
    Scene* StartSimulation();
    char* textBoxBuffer = nullptr;
    int curentSize ;
    const int maxSize = 128;
    bool textBoxEditMode, wasLoad;

public:
    ViewScene(Map* map);
    void init(std::string name);
    ViewScene(Map* map, std::string name);
    ~ViewScene();
    Scene* DrawControl() override;
};
