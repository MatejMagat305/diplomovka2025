#pragma once

#include "scene.h"
#include "map.h"

class LoadScene : public Scene {
private:
    std::vector<std::string> savedMaps;
    int selectedIndex;
    Vector2 scrollOffset = { 0, 0 };
    Rectangle scrollPanelBounds;
    Rectangle contentBounds;
    Rectangle backButtonBounds;
    Rectangle loadButtonBounds;
    Rectangle view;
public:

    LoadScene();

    Scene* DrawControl() override;

};