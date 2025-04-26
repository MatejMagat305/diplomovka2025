#include "infoScene.h"
#include "simulationScene.h"
#include "mainScene.h"
#include "raygui.h"

#include <sstream>

InfoScene::InfoScene(MemSimulation* s0): mem(s0){
}

Scene* InfoScene::DrawControl(){
	Map& m = *(mem->map);
	if (m.CPUMemory.minSize_numberColision_error[1] != 0){
		DrawText("The simulation is not able to solve", 50, GetWindowHeight() / 2, 40, RED);
		DrawText(", press SPACE to get back to main menu", 50, GetWindowHeight() / 2+50, 40, RED);
		if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_A)) {
			return new MainScene();
		}
	}
	else {
		m.draw2(GetWindowWidth(), GetWindowHeight() - 40, 3);
		DrawText("Press SPACE to get back to the simulation", 50, GetWindowHeight() / 2, 20, RED);
		std::stringstream ss;
		ss << "total time: " << mem->i.timeRun + mem->i.timeSynchronize << " ms";
		DrawText(ss.str().c_str(), 50, GetWindowHeight() / 2 + 30, 20, RED);
		ss = std::stringstream();
		ss << "compute time: " << mem->i.timeRun << " ms";
		DrawText(ss.str().c_str(), 50, GetWindowHeight() / 2 + 50, 20, RED);
		ss = std::stringstream();
		ss << "synchronize time: " << mem->i.timeSynchronize << " ms";
		DrawText(ss.str().c_str(), 50, GetWindowHeight() / 2 + 70, 20, RED);

		if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_A)) {
			Scene* r = new SimulationScene(mem);
			mem = nullptr;
			return r;
		}
	}
	return this;
}

InfoScene::~InfoScene() {
	if (mem != nullptr) {
		delete mem;
	}
}
