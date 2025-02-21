#include "changeScene.h"
#include "raygui.h"
#include "mainScene.h"
#include "viewScene.h"
#include <algorithm> 

Scene* ChangeScene::DrawControl() {
    map->draw(GetWindowWidth(), GetWindowHeight() - 50);
    HandleGridClick();
    DrawControlButtons();
    return this;
}

ChangeScene::~ChangeScene() {
    if (map != nullptr) {
        delete map;
    }
}

void ChangeScene::HandleGridClick() {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        int cellWidth = GetWindowWidth() / map->width;
        int cellHeight = (GetWindowHeight() - 50) / map->height;
        int x = static_cast<int>(mousePos.x / cellWidth);
        int y = static_cast<int>(mousePos.y / cellHeight);
        if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
            if (selected.x != -1) {
                SwapGridCells(x, y);
            } else {
                selected = Position{ x, y };
            }
        }
    }
}

void ChangeScene::SwapGridCells(int x, int y) {
    int prevX = selected.x;
    int prevY = selected.y;
    std::swap(map->grid[y][x], map->grid[prevY][prevX]);
    selected = Position{ -1,-1 };
}

Scene* ChangeScene::DrawControlButtons() {
    if (GuiButton(Rectangle{ 160, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "New")) {
        map->reset();
    }

    if (GuiButton(Rectangle{ 270, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Back")) {
        Scene* mainScene = new MainScene();
        return mainScene;
    }

    if (selected.x != -1) { 
        if (GuiButton(Rectangle{ 380, static_cast<float>(GetWindowHeight()) - 40, 120, 30 }, "Add Agent")) {
            AddObjectToMap(AGENT_SYMBOL);
        }

        if (GuiButton(Rectangle{ 510, static_cast<float>(GetWindowHeight()) - 40, 120, 30 }, "Add Obstacle")) {
            AddObjectToMap(OBSTACLE_SYMBOL);
        }

        if (GuiButton(Rectangle{ 640, static_cast<float>(GetWindowHeight()) - 40, 120, 30 }, "Add Loader")) {
            AddObjectToMap(LOADER_SYMBOL);
        }

        if (GuiButton(Rectangle{ 770, static_cast<float>(GetWindowHeight()) - 40, 120, 30 }, "Add Unloader")) {
            AddObjectToMap(UNLOADER_SYMBOL);
        }
    }
    else {
        GuiDisable(); 
        GuiButton(Rectangle{ 380, static_cast<float>(GetWindowHeight()) - 40, 120, 30 }, "Add Agent");
        GuiButton(Rectangle{ 510, static_cast<float>(GetWindowHeight()) - 40, 120, 30 }, "Add Obstacle");
        GuiButton(Rectangle{ 640, static_cast<float>(GetWindowHeight()) - 40, 120, 30 }, "Add Loader");
        GuiButton(Rectangle{ 770, static_cast<float>(GetWindowHeight()) - 40, 120, 30 }, "Add Unloader");
        GuiEnable();
    }

    if (GuiButton(Rectangle{ 490, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "View")) {
        Scene* viewScene = new ViewScene(map);
        map = nullptr;
        return viewScene;
    }
}
void ChangeScene::AddObjectToMap(char objectType) {
    if (selected.x == -1) return;
    int x = selected.x;
    int y = selected.y;
    if (map->grid[x][y] == )
    {

    }
    switch (objectType) {
    case AGENT_SYMBOL:
        AddAgent(x, y);
        break;
    case OBSTACLE_SYMBOL:
        map->grid[y][x] = OBSTACLE_SYMBOL;
        map->obstacleCount++;
        break;
    case LOADER_SYMBOL:
        map->grid[y][x] = LOADER_SYMBOL;
        AddLoader(x, y);
        break;
    case UNLOADER_SYMBOL:
        map->grid[y][x] = UNLOADER_SYMBOL;
        AddUnloader(x, y);
        break;
    default:
        break;
    }
}

void ChangeScene::AddAgent(int x, int y) {
    Agent* newAgents = new Agent[map->agentCount + 1];
    std::copy(map->agents, map->agents + map->agentCount, newAgents);
    newAgents[map->agentCount] = Agent(x, y);

    delete[] map->agents;
    map->agents = newAgents;
    map->agentCount++;
}

void ChangeScene::AddLoader(int x, int y) {
    Position* newLoaders = new Position[map->loaderCount + 1];
    std::copy(map->loaderPosition, map->loaderPosition + map->loaderCount, newLoaders);
    newLoaders[map->loaderCount] = { x, y };

    delete[] map->loaderPosition;
    map->loaderPosition = newLoaders;
    map->loaderCount++;
}

void ChangeScene::AddUnloader(int x, int y) {
    Position* newUnloaders = new Position[map->unloaderCount + 1];
    std::copy(map->unloaderPosition, map->unloaderPosition + map->unloaderCount, newUnloaders);
    newUnloaders[map->unloaderCount] = { x, y };

    delete[] map->unloaderPosition;
    map->unloaderPosition = newUnloaders;
    map->unloaderCount++;
}
