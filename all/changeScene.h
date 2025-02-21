#pragma once

#include "scene.h"
#include "map.h"

class ChangeScene : public Scene{
	Map *map = nullptr;
    void HandleGridClick();
    void SwapGridCells(int x, int y);
    void AddObjectToMap(char objectType);
    void AddAgent(int x, int y);
    void AddLoader(int x, int y);
    void AddUnloader(int x, int y);
    Scene* DrawControlButtons();
public:
	ChangeScene(Map* map): map(map){}

	Scene* DrawControl() override;
	~ChangeScene();
};

