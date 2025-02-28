#include "infoScene.h"
#include "simulationScene.h"
#include "raygui.h"

#include <sstream>

InfoScene::InfoScene(Map* mm, Info ii): map(mm), i(ii){}

Scene* InfoScene::DrawControl(){
	map->draw2(GetWindowWidth(), GetWindowHeight() - 50, 3);
	DrawText("Press SPACE to get back to the simulation", 50, GetWindowHeight()/2, 20, RED);
	std::stringstream ss;
	ss << "total time: " << i.timeRun + i.timeSynchronize << " ms";
	DrawText(ss.str().c_str(), 50, GetWindowHeight() / 2 + 30, 20, RED);
	ss = std::stringstream();
	ss << "compute time: " << i.timeRun << " ms";
	DrawText(ss.str().c_str(), 50, GetWindowHeight() / 2 + 50, 20, RED);
	ss = std::stringstream();
	ss << "synchronize time: " << i.timeSynchronize << " ms";
	DrawText(ss.str().c_str(), 50, GetWindowHeight() / 2 + 70, 20, RED);

	if (IsKeyPressed(KEY_SPACE)){
		Scene* result =  new SimulationScene(map);
		map = nullptr;
		return result;
	}
	return this;
}

InfoScene::~InfoScene() {
    if (map != nullptr) {
        delete map;
    }
}
