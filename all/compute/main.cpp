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
        if (pathSize == minSize){
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

    std::unordered_set<std::string> filesToTest;

    // Testovanie rôznych veľkostí máp s a8 a 25% prekážkami
    std::vector<int> sizes = { 16, 32, 64, 128, 256, 512, 1024 };
    for (int s : sizes) {
        int area = s * s;
        int o = area / 4; // 25 %
        filesToTest.insert("map_" + std::to_string(s) + "x" + std::to_string(s) + "_a8_o" + std::to_string(o) + ".data");
        filesToTest.insert("map_" + std::to_string(s) + "x" + std::to_string(s) + "_a7_o" + std::to_string(o) + ".data");
    }

    // Testovanie 1024x1024 a64 pri rôznych počtoch prekážok (0% až 25%)
    for (int frac = 0; frac <= 4; ++frac) {
        int o = (1024 * 1024 * frac) / 16;
        filesToTest.insert("map_1024x1024_a64_o" + std::to_string(o) + ".data");
        filesToTest.insert("map_1024x1024_a63_o" + std::to_string(o) + ".data");
    }
    /*
    // gpu only 
    // Testovanie pre 1024x1024 a viac agentov
    std::vector<int> agents = { 3, 4, 7, 8, 15, 16, 31, 32, 63, 64 };
    int maxO = (1024 * 1024 * 4) / 16;
    for (int ag : agents) {
        filesToTest.insert("map_1024x1024_a" + std::to_string(ag) + "_o" + std::to_string(maxO) + ".data");
    }
    */
    if (filesToTest.empty()) {
        std::cerr << "⚠️  Warning: filesToTest is empty. No maps will be processed.\n";
        return 1;
    }

    std::vector<ComputeType> methods = {
        ComputeType::highProcesor,
        ComputeType::pureProcesorOneThread,
        ComputeType::pureProcesor,
        ComputeType::hybridGPUCPU,
        ComputeType::pureGraphicCard
    };

    for (const auto& entry : fs::directory_iterator(inputDir)) {
        std::string filename = entry.path().filename().string();

        if (filesToTest.find(filename) == filesToTest.end()) {
            continue; // nie je v zozname testovaných
        }

        Map map = Map();

        try {
            map.load(filename.c_str());


            for (int i = 0;methods.size()>i;i++) {
				ComputeType method = methods[i];
                long long totalRun = 0;
                long long totalSync = 0;
                long long totalPathSize = 0;
                long long totalGoalAchieve = 0;
                std::string error = "";

                for (int i = 0; i < REPEATS; ++i) {
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
