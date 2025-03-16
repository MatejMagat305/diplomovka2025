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
            }
            else {
                map->selected = Position{ x, y };
            }
        }
    }
}

void ChangeScene::SwapGridCells(int x, int y) {
    int prevX = map->selected.x;
    int prevY = map->selected.y;
    Position prev = map->selected, current = Position{ x,y };
    map->selected = Position{ -1,-1 };
    controlSwapAgent(current, prev);
    if (map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] == FREEFIELD_SYMBOL &&
        map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, prevX, prevY)] == FREEFIELD_SYMBOL) {
        return;
    }
    controlSwapLoader(current, prev);
    controlSwapUnloader(current, prev);
    std::swap(map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)],
        map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, prevX, prevY)]);
}

void ChangeScene::controlSwapAgent(Position current, Position prev) {
    int x = current.x, y = current.y, prevX = prev.x, prevY = prev.y;
    Agent* agents = map->CPUMemory.agents;
    for (int i = 0; i < map->CPUMemory.agentsCount; i++) {
        Agent& a = agents[i];
        int agentX = a.x, agentY = a.y;
        if (x == agentX && y == agentY) {
            a.x = prevX;
            a.y = prevY;
            AgentFrame& f = map->agentFrames[i];
            f.x = static_cast<float>(a.x);
            f.y = static_cast<float>(a.y);
            continue;
        }
        if (prevX == agentX && prevY == agentY) {
            a.x = x;
            a.y = y;
            AgentFrame& f = map->agentFrames[i];
            f.x = static_cast<float>(a.x);
            f.y = static_cast<float>(a.y);
        }
    }
}

void ChangeScene::controlSwapLoader(Position current, Position prev) {
    int x = current.x, y = current.y, prevX = prev.x, prevY = prev.y;
    Position* loaders = map->CPUMemory.loaderPositions;
    for (int i = 0; i < map->CPUMemory.loadersCount; i++) {
        Position& p = loaders[i];
        int loaderX = p.x, loaderY = p.y;
        if (x == loaderX && y == loaderY) {
            p.x = prevX;
            p.y = prevY;
            continue;
        }
        if (prevX == loaderX && prevY == loaderY) {
            p.x = x;
            p.y = y;
        }
    }
}

void ChangeScene::controlSwapUnloader(Position current, Position prev) {
    int x = current.x, y = current.y, prevX = prev.x, prevY = prev.y;
    Position* unloaders = map->CPUMemory.unloaderPositions;
    for (int i = 0; i < map->CPUMemory.unloadersCount; i++) {
        Position& p = unloaders[i];
        int loaderX = p.x, loaderY = p.y;
        if (x == loaderX && y == loaderY) {
            p.x = prevX;
            p.y = prevY;
            continue;
        }
        if (prevX == loaderX && prevY == loaderY) {
            p.x = x;
            p.y = y;
        }
    }
}

Scene* ChangeScene::DrawControlButtons() {
    const int sizeButton = 85, spaceButton = 5;
    GuiEnable();
    if (GuiButton(Rectangle{ 330 - (spaceButton + sizeButton + 10)*3, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "New")) {
        map->reset();
    }

    if (GuiButton(Rectangle{ 330 - (spaceButton + sizeButton + 10)*2, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Back")) {
        Scene* mainScene = new MainScene();
        return mainScene;
    }

    if (GuiButton(Rectangle{ 330 - (spaceButton + sizeButton + 10), static_cast<float>(GetWindowHeight()) - 40, sizeButton, 30 }, "View")) {
        Scene* viewScene = new ViewScene(map);
        map->selected = Position{ -1,-1 };
        map = nullptr;
        return viewScene;
    }
    Position& selected = map->selected;

    bool buttonsEnabled = (selected.x != -1 && map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, selected.x, selected.y)] == FREEFIELD_SYMBOL) && !map->isAgentIn(selected.x, selected.y, map->CPUMemory.agentsCount);

    if (!buttonsEnabled) {
        GuiDisable();
    }

    if (GuiButton(Rectangle{ 330, static_cast<float>(GetWindowHeight()) - 40, sizeButton, 30}, "Add Obstacle")) {
         AddObjectToMap(OBSTACLE_SYMBOL);
    }

    if (GuiButton(Rectangle{ 330 + (spaceButton + sizeButton), static_cast<float>(GetWindowHeight()) - 40, sizeButton, 30 }, "Add Loader")) {
         AddObjectToMap(LOADER_SYMBOL);
    }

    if (GuiButton(Rectangle{ 330 + 2 * (spaceButton + sizeButton), static_cast<float>(GetWindowHeight()) - 40, sizeButton, 30 }, "Add Unloader")) {
        AddObjectToMap(UNLOADER_SYMBOL);
    }

    if (GuiButton(Rectangle{ 330 + 3 * (spaceButton + sizeButton), static_cast<float>(GetWindowHeight()) - 40, sizeButton, 30 }, "Add Agent")) {
        AddObjectToMap(AGENT_SYMBOL);
    }

    if (!buttonsEnabled) {
        GuiEnable();
    }

    if (buttonsEnabled || selected.x == -1) {
        GuiDisable();
    }
    if (GuiButton(Rectangle{ 330 + 4 * (spaceButton + sizeButton), static_cast<float>(GetWindowHeight()) - 40, sizeButton, 30 }, "Delete Object")) {
        deleteObjectToMap();
    }

    GuiEnable();
    return this;
}

void ChangeScene::AddObjectToMap(char objectType) {
    if (map->selected.x == -1) return;
    int x = map->selected.x;
    int y = map->selected.y;
    if (map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] != FREEFIELD_SYMBOL) {
        return;
    }
    if (map->isAgentIn(x, y, map->CPUMemory.agentsCount)) {
        return;
    }
    switch (objectType) {
    case AGENT_SYMBOL:
        map->AddAgent(x, y);
        break;
    case OBSTACLE_SYMBOL:
        map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] = OBSTACLE_SYMBOL;
        map->obstacleCount++;
        break;
    case LOADER_SYMBOL:
        map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] = LOADER_SYMBOL;
        map->AddLoader(x, y);
        break;
    case UNLOADER_SYMBOL:
        map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] = UNLOADER_SYMBOL;
        map->AddUnloader(x, y);
        break;
    default:
        break;
    }
    map->selected = Position{ -1,-1 };
}

void ChangeScene::deleteObjectToMap() {
    if (map->selected.x == -1) return;
    int x = map->selected.x;
    int y = map->selected.y;
    map->selected = {-1,-1};
    if (map->isAgentIn(x, y, map->CPUMemory.agentsCount)) {
        map->RemoveAgent(x, y);
        return;
    }

    if (map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] == OBSTACLE_SYMBOL) {
        map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] = FREEFIELD_SYMBOL;
        map->obstacleCount--;
        return;
    }

    if (map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] == LOADER_SYMBOL) {
        map->RemoveLoader(x, y);
        return;
    }

    if (map->CPUMemory.grid[getTrueIndexGrid(map->CPUMemory.width, x, y)] == UNLOADER_SYMBOL) {
        map->RemoveUnloader(x, y);
        return;
    }
}