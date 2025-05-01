/*
#pragma once

#ifndef SCENE_H
#define SCENE_H
extern float screenWidth;
extern float screenHeight;

class Scene {
public:
    virtual Scene* DrawControl() = 0;
    virtual ~Scene() = default;

    static float GetWindowHeight() { return screenHeight; }
    static float GetWindowWidth() { return screenWidth; }
};

#endif
class MainScene : public Scene {
public:
    MainScene();
    Scene* DrawControl() override;
    ~MainScene();
};
class ParameterScene : public Scene {
    int mapWidth = 10;
    int mapHeight = 10;
    int agentCount = 3;
    int obstacleCount = 7;
    int loaderCount = 2;
    int unloaderCount = 2;

    bool activeWidth = false;
    bool activeHeight = false;
    bool activeAgents = false;
    bool activeObstacles = false;
    bool activeMarkers = false;
    bool activeLoaders = false;
    bool activeUnloaders = false;
    bool alert = false;
public:
    ParameterScene() = default;
    Scene* DrawControl() override;
    ~ParameterScene() = default;
};
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
#pragma once

#include <mutex>
#include <thread>
#include <atomic>
#include "device_type_algoritmus.h"
#include <map>
#include <vector>
#include "map.h"

struct MemSimulation {
    int indexType;
    std::string buttonMsg;
    std::map<ComputeType, std::string> stringMap{};
    std::vector<ComputeType> method{};
    bool hasInfo = false, isCPUOverGPU = false;
    Info i;
    std::mutex simMutex;
    std::thread* simThread = nullptr;
    std::atomic<bool> isRunning = false;
    Map* map;

    ~MemSimulation() {
        sweepThread();
        if (map != nullptr) {
            delete map;
        }
    }
    void sweepThread() {
        if (simThread != nullptr) {
            if (simThread->joinable()) {
                simThread->join();
            }
            delete simThread;
        }
    }
};
#pragma once


#include "scene.h"
#include "map.h"

class ViewScene : public Scene {
private:
    Map* map;
    Scene* StartSimulation();
    char* textBoxBuffer = nullptr;
    int curentSize;
    const int maxSize = 128;
    bool textBoxEditMode, wasLoad;

public:
    ViewScene(Map* map);
    void init(std::string name);
    ViewScene(Map* map, std::string name);
    ~ViewScene();
    Scene* DrawControl() override;
};
#pragma once
#include "scene.h"
#include "map.h"
#include "memSimulation.h"
#include <mutex>



class SimulationScene : public Scene {
private:
    void switchComputeType();
    void runSimulation();
    MemSimulation* mem;
    std::mutex mtx;
    bool shouldEnd = false;

public:
    SimulationScene(Map* map);
    SimulationScene(MemSimulation* mem);
    Scene* DrawControl() override;
    ~SimulationScene();
    void tik();
};
#pragma once
#include "scene.h"
#include "device_type_algoritmus.h"
#include "memSimulation.h"


class InfoScene : public Scene {
private:
    MemSimulation* mem;

public:
    InfoScene(MemSimulation* s0);
    Scene* DrawControl() override;
    ~InfoScene();
};

#pragma once

#include "scene.h"
#include "map.h"

class ChangeScene : public Scene {
    Map* map = nullptr;
    void HandleGridClick();
    void SwapGridCells(int x, int y);
    void controlSwapAgent(Position current, Position prev);
    void controlSwapLoader(Position current, Position prev);
    void controlSwapUnloader(Position current, Position prev);
    void AddObjectToMap(char objectType);
    void deleteObjectToMap();
    Scene* DrawControlButtons();
public:
    ChangeScene(Map* map) : map(map) {}

    Scene* DrawControl() override;
    ~ChangeScene();
};
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
        int agentX = a.position.x, agentY = a.position.y;
        if (x == agentX && y == agentY) {
            a.position = Position{ x,y };
            AgentFrame& f = map->agentFrames[i];
            f.x = static_cast<float>(a.position.x);
            f.y = static_cast<float>(a.position.y);
            continue;
        }
        if (prevX == agentX && prevY == agentY) {
            a.position = Position{ x,y };
            AgentFrame& f = map->agentFrames[i];
            f.x = static_cast<float>(a.position.x);
            f.y = static_cast<float>(a.position.y);
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
    if (GuiButton(Rectangle{ 330 - (spaceButton + sizeButton + 10) * 3, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "New")) {
        map->reset();
    }

    if (GuiButton(Rectangle{ 330 - (spaceButton + sizeButton + 10) * 2, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Back")) {
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

    if (GuiButton(Rectangle{ 330, static_cast<float>(GetWindowHeight()) - 40, sizeButton, 30 }, "Add Obstacle")) {
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
    map->selected = { -1,-1 };
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

#include "infoScene.h"
#include "simulationScene.h"
#include "mainScene.h"
#include "raygui.h"

#include <sstream>

InfoScene::InfoScene(MemSimulation* s0) : mem(s0) {
}

Scene* InfoScene::DrawControl() {
    Map& m = *(mem->map);
    if (m.CPUMemory.minSize_numberColision_error[1] != 0) {
        DrawText("The simulation is not able to solve", 50, GetWindowHeight() / 2, 40, RED);
        DrawText(", press SPACE to get back to main menu", 50, GetWindowHeight() / 2 + 50, 40, RED);
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
#include "loadScene.h"
#include "mainScene.h"
#include "map.h"
#include "raygui.h"
#include "viewScene.h"

LoadScene::LoadScene() {
    std::vector<std::string> maps = getSavedFiles("saved_map", ".data");
    savedMaps.clear();
    int size = maps.size();
    savedMaps.reserve(size);
    for (int i = 0; i < size; i++) {
        std::string& s = maps[i];
        if (s != "") {
            savedMaps.push_back(s);
        }
    }
    scrollPanelBounds = { 0, 0, (float)screenWidth, (float)screenHeight - 50 };
    contentBounds = { 0, 0, (float)screenWidth - 20, (float)(savedMaps.size() * 30) };
    backButtonBounds = { 10, (float)screenHeight - 40, 100, 30 };
    loadButtonBounds = { (float)screenWidth - 110, (float)screenHeight - 40, 100, 30 };
    view = { 0, 0, scrollPanelBounds.width, scrollPanelBounds.height };
}

Scene* LoadScene::DrawControl() {
    GuiScrollPanel(scrollPanelBounds, "select", contentBounds, &scrollOffset, &view);
    BeginScissorMode((int)scrollPanelBounds.x, (int)scrollPanelBounds.y,
        (int)scrollPanelBounds.width, (int)scrollPanelBounds.height);

    for (size_t i = 0; i < savedMaps.size(); i++) {
        Rectangle mapNameBounds = {
            scrollPanelBounds.x + 10,
            scrollPanelBounds.y + 50 + (float)(i * 30) - scrollOffset.y,
            scrollPanelBounds.width - 20, 25
        };

        if (CheckCollisionPointRec(GetMousePosition(), mapNameBounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            selectedIndex = i;
        }
        if (i == selectedIndex) {
            DrawRectangleRec(mapNameBounds, LIGHTGRAY);
        }
        DrawText(savedMaps[i].c_str(), (int)mapNameBounds.x, (int)mapNameBounds.y, 20, DARKGRAY);
    }

    EndScissorMode();
    if (GuiButton(backButtonBounds, "Back")) {
        return new MainScene();
    }
    if (GuiButton(loadButtonBounds, "Load")) {
        if (selectedIndex >= 0 && selectedIndex < savedMaps.size()) {
            Map* m = new Map();
            m->load(savedMaps[selectedIndex]);
            ViewScene* v = new ViewScene(m, savedMaps[selectedIndex]);
            return v;
        }
    }
    return this;
}
#include "mainScene.h"

#include "loadScene.h"
#include "parameterScene.h"
#include "raylib.h"
#include "raygui.h"

MainScene::MainScene() = default;

MainScene::Scene* MainScene::DrawControl() {
    DrawText("Main Menu", 350, 100, 20, DARKGRAY);

    if (GuiButton(Rectangle{ 300, 200, 200, 50 }, "New Map")) {
        return new ParameterScene();

    }

    if (GuiButton(Rectangle{ 300, 300, 200, 50 }, "Load Map")) {
        return new LoadScene();

    }
    return this;
}

MainScene::~MainScene() = default;


#include <iostream>
#include <sstream>
#include <string>

#include "raylib.h"
#include "map.h"
#include "raygui.h"

#include "scene.h"
#include "parameterScene.h"
#include "viewScene.h"

Scene* ParameterScene::DrawControl() {
    DrawText("Set Parameters", 350, 50, 20, DARKGRAY);

    if (GuiValueBox({ 300, 100, 200, 20 }, "Width", &mapWidth, 5, 50, activeWidth)) {
        activeWidth = !activeWidth;
        alert = activeHeight = activeAgents = activeObstacles = activeMarkers = activeLoaders = activeUnloaders = false;
    }
    if (GuiValueBox({ 300, 140, 200, 20 }, "Height", &mapHeight, 5, 50, activeHeight)) {
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
    if (alert) {
        std::stringstream ss;
        ss << "minimal width is " << 5 << ", minimal height is 5, minimal numbers of agents, loaders and unloader are 1";
        std::string s = ss.str();
        DrawText(s.c_str(), 300, 470, 20, RED);
    }

    return this;
}
#include "simulationScene.h"
#include "viewScene.h"
#include "raygui.h"
#include "infoScene.h"
#include "compute.h"
#include "fill_init_convert.h"

void SimulationScene::switchComputeType() {
    mem->indexType = (mem->indexType + 1) % mem->method.size();
    mem->buttonMsg = mem->stringMap[mem->method[mem->indexType]];
}

void SimulationScene::runSimulation() {
    if (mem->isRunning) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mem->simMutex);
        if (mem->isRunning) {
            return;
        }
        mem->isRunning = true;
    }
    mem->sweepThread();
    mem->simThread = new std::thread([this]() {
        Map* m = mem->map;
        mem->i = letCompute(mem->method[mem->indexType], m);
        mem->hasInfo = true;
        });
}

SimulationScene::SimulationScene(Map* map) {
    mem = new MemSimulation();
    mem->map = map;
    mem->method.reserve(5);
    mem->method.emplace_back(ComputeType::pureProcesor);
    mem->method.emplace_back(ComputeType::pureProcesorOneThread);
    mem->method.emplace_back(ComputeType::pureGraphicCard);
    mem->method.emplace_back(ComputeType::hybridGPUCPU);
    mem->method.emplace_back(ComputeType::highProcesor);
    mem->indexType = 0;
    mem->buttonMsg = "repeat A* gpu algoritmus on cpu";
    mem->stringMap.emplace(ComputeType::pureProcesor, mem->buttonMsg);
    mem->stringMap.emplace(ComputeType::pureProcesorOneThread, "repeat A* gpu algoritmus on cpu 1 thread");
    mem->stringMap.emplace(ComputeType::pureGraphicCard, "gpu A* algoritmus");
    mem->stringMap.emplace(ComputeType::highProcesor, "CBS algoritmus on only cpu");
    mem->stringMap.emplace(ComputeType::hybridGPUCPU, "CBS algoritmus on combination gpu and cpu");
    initializeSYCLMemory(map);
    synchronizeGPUFromCPU(map);
}

SimulationScene::SimulationScene(MemSimulation* mem0) : mem(mem0) {
    mem->sweepThread();
    mem->hasInfo = false;
    mem->simThread = new std::thread([this]() {
        tik();
        mem->isRunning = false;
        });
}

Scene* SimulationScene::DrawControl() {
    mtx.lock();
    mem->map->draw2(GetWindowWidth(), GetWindowHeight() - 40, 1);
    mtx.unlock();
    if (GuiButton(Rectangle{ 50, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Back")) {
        mem->isRunning = false;
        shouldEnd = true;
        Scene* r = new ViewScene(mem->map);
        mem->map = nullptr;
        return r;

    }
    if (!mem->isRunning && GuiButton(Rectangle{ 270, static_cast<float>(GetWindowHeight()) - 40, 300, 30 }, mem->buttonMsg.c_str())) {
        switchComputeType();
    }

    if (!mem->isRunning && GuiButton(Rectangle{ 160, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Run")) {
        runSimulation();
    }
    if (mem->hasInfo) {
        Scene* r = new InfoScene(mem);
        mem = nullptr;
        return r;
    }
    return this;
}
SimulationScene::~SimulationScene() {
    if (mem != nullptr) {
        shouldEnd = true;
        delete mem;
        deleteGPUMem();
    }
}

void SimulationScene::tik() {
    MemoryPointers& globalMemory = mem->map->CPUMemory;
    Map* map = mem->map;
    int size = globalMemory.minSize_numberColision_error[0];
    int agentsCount = globalMemory.agentsCount;
    const int mapSize = globalMemory.width * globalMemory.height;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    int step = 60;
    for (int i = 1; i < size; i++) {
        for (int j = 0; j < step; j++) {
            mtx.lock();
            for (int agentID = 0; agentID < agentsCount; agentID++) {
                int offset = mapSize * agentID;
                Position* agentPath = &(globalMemory.agentPaths[offset]);
                Position current = agentPath[i - 1];
                Position next = agentPath[i];
                float deltaX = static_cast<float>((static_cast<double>(next.x - current.x) * static_cast<double>(j)) / static_cast<double>(step));
                float deltaY = static_cast<float>((static_cast<double>(next.y - current.y) * static_cast<double>(j)) / static_cast<double>(step));
                AgentFrame& frame = map->agentFrames[agentID];
                frame.x = (float)current.x + deltaX;
                frame.y = (float)current.y + deltaY;
            }
            mtx.unlock();
            if (shouldEnd) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(400) / step);
            if (shouldEnd) {
                return;
            }
        }
    }

    if (1 == size) {
        for (int j = 0; j < step; j++) {
            mtx.lock();
            for (int agentID = 0; agentID < agentsCount; agentID++) {
                int offset = mapSize * agentID;
                Position* agentPath = &(globalMemory.agentPaths[offset]);
                Position current = globalMemory.agents[agentID].position;
                Position next = agentPath[0];
                float deltaX = static_cast<float>((static_cast<double>(next.x - current.x) * static_cast<double>(j)) / static_cast<double>(step));
                float deltaY = static_cast<float>((static_cast<double>(next.y - current.y) * static_cast<double>(j)) / static_cast<double>(step));
                AgentFrame& frame = map->agentFrames[agentID];
                frame.x = (float)current.x + deltaX;
                frame.y = (float)current.y + deltaY;
            }
            mtx.unlock();
            if (shouldEnd) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(400) / step);
            if (shouldEnd) {
                return;
            }
        }
    }
}
#include "viewScene.h"
#include "mainScene.h"
#include "parameterScene.h"
#include "simulationScene.h"
#include "raygui.h"
#include "changeScene.h"

Scene* ViewScene::DrawControl() {
    map->draw(GetWindowWidth(), GetWindowHeight() - 50);

    // Textové pole
    if (GuiTextBox(Rectangle{ 650, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, textBoxBuffer, maxSize - 1, textBoxEditMode)) {
        textBoxEditMode = !textBoxEditMode; // Zmena režimu editácie pri kliknutí
    }

    if (!wasLoad && GuiButton(Rectangle{ 50, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Save and start")) {
        map->save(textBoxBuffer[0] != '\0' ? textBoxBuffer : "world"); // Uloženie s názvom z textového po¾a
        return StartSimulation();
    }

    if (GuiButton(Rectangle{ 160, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "New")) {
        map->reset();
        return this;
    }

    if (GuiButton(Rectangle{ 270, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Back")) {
        return new MainScene();
    }

    if (GuiButton(Rectangle{ 380, static_cast<float>(GetWindowHeight()) - 40, 130, 30 }, "Start without save")) {
        return StartSimulation();
    }

    if (GuiButton(Rectangle{ 520, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Change")) {
        Scene* result = new ChangeScene(map);
        map = nullptr;
        return result;
    }
    return this;
}

Scene* ViewScene::StartSimulation() {
    SimulationScene* result = new SimulationScene(map);
    map = nullptr;
    return result;
}

ViewScene::~ViewScene() {
    if (map != nullptr) {
        delete map;
    }
    delete[] textBoxBuffer;
}

ViewScene::ViewScene(Map* map) : map(map) {
    init("world");
}

void ViewScene::init(std::string name) {
    textBoxBuffer = new char[maxSize];
    std::copy(name.cbegin(), name.cend(), textBoxBuffer);
    textBoxBuffer[name.length()] = '\0';
    curentSize = name.length() + 1;
    textBoxEditMode = false;
    wasLoad = false;
}

ViewScene::ViewScene(Map* map, std::string name) : map(map) {
    std::string nameWithoutExt = std::string(GetFileNameWithoutExt(name.c_str()));
    init(nameWithoutExt);
    wasLoad = true;
}

*/