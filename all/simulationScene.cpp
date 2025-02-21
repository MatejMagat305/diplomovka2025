
#include "simulationScene.h"
#include "viewScene.h"
#include "raygui.h"
#include <mutex>


void SimulationScene::simulateStep()
{
}

Scene* SimulationScene::DrawControl() {
    map->draw(GetWindowWidth(), GetWindowHeight() - 50);

    if (GuiButton(Rectangle { 50, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Back")) {
        Scene* r = new ViewScene(map);
        map = nullptr;
        return r;
    }
    return this;
}

SimulationScene::~SimulationScene(){
    if (map != nullptr){
        delete map;
    }
}

