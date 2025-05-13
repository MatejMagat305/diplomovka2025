#include "map.h"
#include <string>
#include <vector>
#include <iostream>
#define RAYGUI_IMPLEMENTATION 
#include "raygui.h"
#include <filesystem>
#include <fstream>
#include "compute.h"

namespace fs = std::filesystem;

#include "mainScene.h"

float screenWidth = 800.;
float screenHeight = 600.;

int init() {
    InitWindow(screenWidth, screenHeight, "Map Generator and Simulation");
    GuiEnable();
    Scene* currentScene = new MainScene();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        Scene* nextScene = currentScene->DrawControl();
        if (nextScene != currentScene) {
            delete currentScene;
            currentScene = nextScene;
        }
        EndDrawing();
    }

    delete currentScene;
    CloseWindow();
    return 0;
}

void computeTotalPathSizeGoalAtchieve(long long& totalGoalAchieve, long long& totalPathSize, Map* mapPtr) {
    MemoryPointers& mem = mapPtr->CPUMemory;
    int minSize = mem.minSize_maxtimeStep_error[MINSIZE];
    for (int i = 0; i < mem.agentsCount; i++) {
        int pathSize = mem.agents[i].sizePath;
        if (pathSize == minSize) {
            totalGoalAchieve++;
        }
        totalPathSize += pathSize;
    }
}
const int REPEATS = 5;

int main() {
    std::string inputDir = "./saved_map/";
    std::ofstream csv("results.csv");
    csv << "File,Method,TimeRun(ms),TimeSynchronize(ms),TotalGoalAchieve,TotalPathSize,Error\n";
    std::vector<ComputeType> methods = {
        ComputeType::highProcesor,
        ComputeType::pureProcesorOneThread,
        ComputeType::pureProcesor,
        ComputeType::hybridGPUCPU,
        ComputeType::pureGraphicCard
    };

    std::unordered_set<std::string> filesToTest;

    // 1. LxL mapy s agentmi 63/64 a prekážkami o = L / 4
    std::vector<int> sizes = { 16, 32, 64, 128, 256, 512, 1024 };
    for (int size : sizes) {
        int o = size / 4;
        filesToTest.insert("map_" + std::to_string(size) + "x" + std::to_string(size) + "_a63_o" + std::to_string(o) + ".data");
        filesToTest.insert("map_" + std::to_string(size) + "x" + std::to_string(size) + "_a64_o" + std::to_string(o) + ".data");
    }

    // 2. 1024x1024, a64, o = 0 po 256
    for (int o = 0; o <= 256; o += 32) { // krok 32 pre menej súborov, uprav si
        filesToTest.insert("map_1024x1024_a64_o" + std::to_string(o) + ".data");
    }

    // 3. 16x16, a3 po a64, bez prekážok
    for (int a = 3; a <= 64; ++a) {
        filesToTest.insert("map_16x16_a" + std::to_string(a) + "_o0.data");
    }
    for (const auto& filename : filesToTest) {
        Map map = Map();

        try {
            map.load(filename.c_str());


            for (int i = 0;methods.size() > i;i++) {
                ComputeType method = methods[i];
                long long totalRun = 0;
                long long totalSync = 0;
                long long totalPathSize = 0;
                long long totalGoalAchieve = 0;
                std::string error = "";
				std::cout << "File: " << filename << " | Method: " << i << "\n";
                for (int j = 0; j < REPEATS; j++) {
                    Map* mapPtr = map.copy();
                    initializeSYCLMemory(mapPtr);
                    synchronizeGPUFromCPU(mapPtr);
                    Info info = letCompute(method, mapPtr);
                    totalRun += info.timeRun;
                    totalSync += info.timeSynchronize;
                    computeTotalPathSizeGoalAtchieve(totalGoalAchieve, totalPathSize, mapPtr);
                    if (!info.error.empty()) error = info.error;
                    deleteGPUMem();
                    delete mapPtr;
                }

                csv << filename << ","
                    << i << ","
                    << (totalRun / REPEATS) << ","
                    << (totalSync / REPEATS) << ","
                    << (totalGoalAchieve / REPEATS) << ","
                    << (totalPathSize / REPEATS) << ","
                    << (error.empty() ? "OK" : error) << "\n";

                std::cout << "File: " << filename
                    << " | Method: " << i
                    << " | AvgTime: " << (totalRun / REPEATS)
                    << " ms | AvgSync: " << (totalSync / REPEATS)
                    << " ms | Paths: " << (totalGoalAchieve / REPEATS)
                    << "/" << map.CPUMemory.agentsCount << "\n";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading map: " << filename << " - " << e.what() << "\n";
        }
    }

    csv.close();
    return 0;
}