#include "changeScene.h"
#include "raygui.h"
#include "mainScene.h"
#include "viewScene.h"
#include "fill_init_convert.h"
#include <algorithm> 
#include "map.h"

Scene* ChangeScene::DrawControl() {
    map->draw(GetWindowWidth(), GetWindowHeight() - 50);
    HandleGridClick();
    return DrawControlButtons();
}

ChangeScene::~ChangeScene() {
    if (map != nullptr) {
        delete map;
    }
}

void ChangeScene::HandleGridClick() {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        int cellWidth = GetWindowWidth() / map->CPUMemory.width;
        int cellHeight = (GetWindowHeight() - 50) / map->CPUMemory.height;
        int x = static_cast<int>(mousePos.x / cellWidth);
        int y = static_cast<int>(mousePos.y / cellHeight);
        if (x >= 0 && x < map->CPUMemory.width && y >= 0 && y < map->CPUMemory.height) {
            if (map->selected.x != -1) {
                SwapGridCells(x, y);
            } else {
                map->selected = Position{ x, y };
            }
        }
    }
}

void ChangeScene::SwapGridCells(int x, int y) {
    int prevX = map->selected.x;
    int prevY = map->selected.y;
    std::swap(map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)], 
        map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, prevX, prevY)]);
    map->selected = Position{ -1,-1 };
}

Scene* ChangeScene::DrawControlButtons() {
    if (GuiButton(Rectangle{ 0, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "New")) {
        map->reset();
    }

    if (GuiButton(Rectangle{ 110, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Back")) {
        Scene* mainScene = new MainScene();
        return mainScene;
    }

    if (map->selected.x != -1) {
        if (GuiButton(Rectangle{ 660, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Add Agent")) {
            AddObjectToMap(AGENT_SYMBOL);
        }

        if (GuiButton(Rectangle{ 330, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Add Obstacle")) {
            AddObjectToMap(OBSTACLE_SYMBOL);
        }

        if (GuiButton(Rectangle{ 440, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Add Loader")) {
            AddObjectToMap(LOADER_SYMBOL);
        }

        if (GuiButton(Rectangle{ 550, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Add Unloader")) {
            AddObjectToMap(UNLOADER_SYMBOL);
        }
    }
    else {
        GuiDisable(); 
        GuiButton(Rectangle{ 660, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Add Agent");
        GuiButton(Rectangle{ 330, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Add Obstacle");
        GuiButton(Rectangle{ 440, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Add Loader");
        GuiButton(Rectangle{ 550, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Add Unloader");
        GuiEnable();
    }

    if (GuiButton(Rectangle{ 220, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "View")) {
        Scene* viewScene = new ViewScene(map);
        map->selected = Position{ -1,-1 };
        map = nullptr;
        return viewScene;
    }
    return this;
}

void ChangeScene::AddObjectToMap(char objectType) {
    if (map->selected.x == -1) return;
    int x = map->selected.x;
    int y = map->selected.y;
    if (map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] != FREEFIELD_SYMBOL){
        return;
    }
    switch (objectType) {
    case AGENT_SYMBOL:
        AddAgent(x, y);
        break;
    case OBSTACLE_SYMBOL:
        map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] = OBSTACLE_SYMBOL;
        map->obstacleCount++;
        break;
    case LOADER_SYMBOL:
        map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] = LOADER_SYMBOL;
        AddLoader(x, y);
        break;
    case UNLOADER_SYMBOL:
        map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] = UNLOADER_SYMBOL;
        AddUnloader(x, y);
        break;
    default:
        break;
    }
}

void ChangeScene::AddAgent(int x, int y) {
    Agent* newAgents = new Agent[map->CPUMemory.agentCount + 1];
    std::copy(map->CPUMemory.agents, map->CPUMemory.agents + map->CPUMemory.agentCount, newAgents);
    newAgents[map->CPUMemory.agentCount] = Agent(x, y);
    delete[] map->CPUMemory.agents;
    map->CPUMemory.agents = newAgents;
    map->CPUMemory.agentCount++;
}

void ChangeScene::AddLoader(int x, int y) {
    Position* newLoaders = new Position[map->CPUMemory.loaderCount + 1];
    std::copy(map->CPUMemory.loaderPosition, map->CPUMemory.loaderPosition + map->CPUMemory.loaderCount, newLoaders);
    newLoaders[map->CPUMemory.loaderCount] = { x, y };
    delete[] map->CPUMemory.loaderPosition;
    map->CPUMemory.loaderPosition = newLoaders;
    map->CPUMemory.loaderCount++;
}

void ChangeScene::AddUnloader(int x, int y) {
    Position* newUnloaders = new Position[map->CPUMemory.unloaderCount + 1];
    std::copy(map->CPUMemory.unloaderPosition, map->CPUMemory.unloaderPosition + map->CPUMemory.unloaderCount, newUnloaders);
    std::copy(map->CPUMemory.unloaderPosition, map->CPUMemory.unloaderPosition + map->CPUMemory.unloaderCount, newUnloaders);
    newUnloaders[map->CPUMemory.unloaderCount] = { x, y };

    delete[] map->CPUMemory.unloaderPosition;
    map->CPUMemory.unloaderPosition = newUnloaders;
    map->CPUMemory.unloaderCount++;
}
