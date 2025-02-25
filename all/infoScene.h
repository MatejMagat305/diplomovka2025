#pragma once
#include "scene.h"
#include "device_type_algoritmus.h"

class InfoScene :  public Scene{
private:
    Map* map;
    Info i;

public:

    InfoScene(Map* mm, Info ii);

    Scene* DrawControl() override;
    ~InfoScene();
};

