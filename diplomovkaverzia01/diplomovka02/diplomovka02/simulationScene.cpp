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
    mem->method.emplace_back(ComputeType::highProcesor);
    mem->method.emplace_back(ComputeType::pureProcesorOneThread);
    mem->method.emplace_back(ComputeType::pureProcesor);
    mem->method.emplace_back(ComputeType::hybridGPUCPU);
    mem->method.emplace_back(ComputeType::pureGraphicCard);
    mem->indexType = 0;
    mem->buttonMsg = "CBS algoritmus on only cpu";
    mem->stringMap.emplace(ComputeType::highProcesor, mem->buttonMsg);
    mem->stringMap.emplace(ComputeType::pureProcesorOneThread, "repeat A* gpu algoritmus on cpu 1 thread");
    mem->stringMap.emplace(ComputeType::pureGraphicCard, "gpu A* algoritmus");
    mem->stringMap.emplace(ComputeType::pureProcesor, "A* gpu algoritmus on cpu (thread per agent)");
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
    mem->map->draw2(GetWindowWidth(), GetWindowHeight()-40, 1);
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
    if (mem->hasInfo){
        Scene* r = new InfoScene(mem);
        mem = nullptr;
        return r;
    }
    return this;
}
SimulationScene::~SimulationScene() {
    if (mem != nullptr){
        shouldEnd = true;
        delete mem;
        deleteGPUMem();
    }
}

void SimulationScene::tik(){
    MemoryPointers& globalMemory = mem->map->CPUMemory;
    Map* map = mem->map;
    int size = globalMemory.minSize_numberColision_error[MINSIZE];
    int agentsCount = globalMemory.agentsCount;
    const int mapSize = globalMemory.width * globalMemory.height;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    int step = 60;
    for (int i = 1; i < size; i++){
        for (int j = 0; j < step; j++){
            mtx.lock();
            for (int agentID = 0; agentID < agentsCount; agentID++){
                int offset = mapSize * agentID;
                Position *agentPath = &(globalMemory.agentPaths[offset]);
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
            std::this_thread::sleep_for(std::chrono::milliseconds(400)/step);
            if (shouldEnd){
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
