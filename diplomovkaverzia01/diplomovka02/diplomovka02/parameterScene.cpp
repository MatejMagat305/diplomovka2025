

#include <iostream>
#include <sstream>
#include <string>

#include "raylib.h"
#include "map.h"
#include "raygui.h"

#include "scene.h"
#include "parameterScene.h"
#include "viewScene.h"

ParameterScene::ParameterScene(){
	mapWidth = 1024;
	mapHeight = 1024;
	agentCount = 3;
	obstacleCount = 7;
	loaderCount = 2;
	unloaderCount = 2;
	activeWidth = false;
	activeHeight = false;
	activeAgents = false;
	activeObstacles = false;
	activeMarkers = false;
	activeLoaders = false;
	activeUnloaders = false;
	alert = false;
}

Scene* ParameterScene::DrawControl(){
    DrawText("Set Parameters", 350, 50, 20, DARKGRAY);

    if (GuiValueBox({ 300, 100, 200, 20 }, "Width", &mapWidth, 5, 1024, activeWidth)) {
        activeWidth = !activeWidth;
        alert = activeHeight = activeAgents = activeObstacles = activeMarkers = activeLoaders = activeUnloaders = false;
    }
    if (GuiValueBox({ 300, 140, 200, 20 }, "Height", &mapHeight, 5, 1024, activeHeight)) {
        activeHeight = !activeHeight;
        alert = activeWidth = activeAgents = activeObstacles = activeMarkers = activeLoaders = activeUnloaders = false;
    }
    if (GuiValueBox({ 300, 180, 200, 20 }, "Agents", &agentCount, 1, 50, activeAgents)) {
        activeAgents = !activeAgents;
        alert = activeWidth = activeHeight = activeObstacles = activeMarkers = activeLoaders = activeUnloaders = false;
    }
    if (GuiValueBox({ 300, 220, 200, 20 }, "Obstacles", &obstacleCount, 1, 100, activeObstacles)) {
        activeObstacles = !activeObstacles;
        alert = activeWidth = activeHeight = activeAgents = activeMarkers = activeLoaders = activeUnloaders = false;
    }
    if (GuiValueBox({ 300, 300, 200, 20 }, "Loaders", &loaderCount, 1, 20, activeLoaders)) {
        activeLoaders = !activeLoaders;
        alert = activeWidth = activeHeight = activeAgents = activeObstacles = activeMarkers = activeUnloaders = false;
    }
    if (GuiValueBox({ 300, 340, 200, 20 }, "Unloaders", &unloaderCount, 1, 20, activeUnloaders)) {
        activeUnloaders = !activeUnloaders;
        alert = activeWidth = activeHeight = activeAgents = activeObstacles = activeMarkers = activeLoaders = false;
    }

    if (GuiButton({ 300, 450, 200, 50 }, "Generate Map")) {
        if (mapWidth > 5 && mapHeight > 5 && agentCount > 0 &&
            loaderCount > 0 && unloaderCount > 0) {
            Map* generatedMap = new Map(mapWidth, mapHeight, agentCount,
                obstacleCount, loaderCount, unloaderCount);
            return new ViewScene(generatedMap);
        }
        else {
            alert = true;
        }
    }
    if (alert){
        std::stringstream ss;
        ss << "minimal width is " << 5 << ", minimal height is 5, minimal numbers of agents, loaders and unloader are 1";
        std::string s = ss.str();
        DrawText(s.c_str(), 300, 470, 20, RED);
    }

    return this;
}
