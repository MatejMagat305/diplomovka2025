#pragma once
#include "scene.h"

class MainScene : public Scene {
public:
    MainScene();
    Scene* DrawControl() override;
    ~MainScene();
};

