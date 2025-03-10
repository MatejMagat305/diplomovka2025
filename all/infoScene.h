#pragma once
#include "scene.h"
#include "device_type_algoritmus.h"
#include "memSimulation.h"


class InfoScene :  public Scene{
private:
    MemSimulation* mem;

public:
    InfoScene(MemSimulation* s0);
    Scene* DrawControl() override;
    ~InfoScene();
};

