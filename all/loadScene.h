#pragma once

#include "scene.h"
#include "map.h"

class LoadScene : public Scene {
private:
    std::vector<std::string> savedMaps;
    int selectedIndex;
    Map* loadedMap;

    void loadSavedMaps();

public:

    LoadScene();

    Scene* DrawControl() override;

};