#include "simulationScene.h"
#include "viewScene.h"
#include "raygui.h"
#include "infoScene.h"
#include "compute.h"

void SimulationScene::switchComputeType() {
    indexType = (indexType + 1) % method.size();
    buttonMsg = stringMap[method[indexType]];
}

void SimulationScene::runSimulation() {
    if (isRunning) {
        return;
    }
    std::lock_guard<std::mutex> lock(simMutex);
    if (isRunning) {
        return;
    }
    isRunning = true;
    simThread = std::thread([this]() {
        i = letCompute(AlgorithmType::ASTAR, method[indexType], *map, 1);
        hasInfo = true;
        });
}

SimulationScene::SimulationScene(Map* map) : map(map) {
    method.reserve(4);
    // method.emplace_back(ComputeType::pureProcesor);
    // method.emplace_back(ComputeType::pureGraphicCard);
    method.emplace_back(ComputeType::hybridGPUCPU);
    method.emplace_back(ComputeType::highProcesor);
    indexType = 0;
    buttonMsg = "CBS algoritmus on gpu-cpu";
    //buttonMsg = "non-optimal gpu algoritmus on cpu";
    // stringMap.emplace(ComputeType::pureProcesor, buttonMsg);
    // stringMap.emplace(ComputeType::pureGraphicCard, "non-optimal gpu algoritmus");
    stringMap.emplace(ComputeType::highProcesor, "CBS algoritmus on only cpu");
    stringMap.emplace(ComputeType::hybridGPUCPU, buttonMsg);
    initializeSYCLMemory(*map);
	synchronizeGPUFromCPU(*map);
}

Scene* SimulationScene::DrawControl() {
    map->draw(GetWindowWidth(), GetWindowHeight() - 50);

    if (GuiButton(Rectangle{ 50, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Back")) {
        isRunning = false;
        if (simThread.joinable()) {
            simThread.join();
        }
        Scene* r = new ViewScene(map);
        map = nullptr;
        return r;
    }
    if (!isRunning && GuiButton(Rectangle{ 270, static_cast<float>(GetWindowHeight()) - 40, 200, 30 }, buttonMsg.c_str())) {
        switchComputeType();
    }

    if (!isRunning && GuiButton(Rectangle{ 160, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Run")) {
        runSimulation();
    }
    if (hasInfo){
        Scene* result = new InfoScene(map, i);
        map = nullptr;
        return result;
    }
    return this;
}

SimulationScene::~SimulationScene() {
    if (simThread.joinable()) {
        simThread.join();
    }
    if (map != nullptr) {
        delete map;
    }
}
