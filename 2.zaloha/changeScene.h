#pragma once

#include "scene.h"
#include "map.h"

class ChangeScene : public Scene{
	Map *map = nullptr;
    void HandleGridClick();
    void SwapGridCells(int x, int y);
    void controlSwapAgent(Position current, Position prev);
    void controlSwapLoader(Position current, Position prev);
    void controlSwapUnloader(Position current, Position prev);
    void AddObjectToMap(char objectType);
    void deleteObjectToMap();
    Scene* DrawControlButtons();
public:
	ChangeScene(Map* map): map(map){}

	Scene* DrawControl() override;
	~ChangeScene();
};

