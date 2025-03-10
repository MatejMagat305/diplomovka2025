#pragma once


#include "scene.h"
#include "map.h"

class ViewScene : public Scene {
private:
    Map* map;
    Scene* StartSimulation();
     char* textBoxBuffer = nullptr; 
     bool textBoxEditMode;

public:
    ViewScene(Map* map);
    ~ViewScene();
    Scene* DrawControl() override;
};
