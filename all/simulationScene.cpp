#include "simulationScene.h"
#include "viewScene.h"
#include "raygui.h"
#include "infoScene.h"
#include "compute.h"

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
    mem->simThread = std::thread([this]() {
        mem->i = letCompute(AlgorithmType::DSTAR, mem->method[mem->indexType], *mem->map);
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
    mem->buttonMsg = "repeat D* gpu algoritmus on cpu 1 thread";
    mem->stringMap.emplace(ComputeType::pureProcesor, mem->buttonMsg);
    mem->stringMap.emplace(ComputeType::pureProcesorOneThread, "repeat D* gpu algoritmus on cpu 1 thread");
    mem->stringMap.emplace(ComputeType::pureGraphicCard, "non-optimal gpu algoritmus");
    mem->stringMap.emplace(ComputeType::highProcesor, "CBS algoritmus on only cpu");
    mem->stringMap.emplace(ComputeType::hybridGPUCPU, "CBS algoritmus on combination gpu and cpu");
    initializeSYCLMemory(*map);
	synchronizeGPUFromCPU(*map);
}

SimulationScene::SimulationScene(MemSimulation* mem) : mem(mem) {}

Scene* SimulationScene::DrawControl() {
    mem->map->draw2(GetWindowWidth(), GetWindowHeight()-40, 1);
    if (GuiButton(Rectangle{ 50, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Back")) {
        mem->isRunning = false;
        if (mem->simThread.joinable()) {
            mem->simThread.join();
        }
        Scene* r = new ViewScene(mem->map);
        mem->map = nullptr;
        return r;

    }
    if (!mem->isRunning && GuiButton(Rectangle{ 270, static_cast<float>(GetWindowHeight()) - 40, 200, 30 }, mem->buttonMsg.c_str())) {
        switchComputeType();
    }

    if (!mem->isRunning && GuiButton(Rectangle{ 160, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Run")) {
        runSimulation();
    }
    if (mem->hasInfo){
        Scene* r = new InfoScene(mem);
        mem = nullptr;
        return r;
    }
    return this;
}
SimulationScene::~SimulationScene() {
    if (mem != nullptr){
        delete mem;
    }
}
