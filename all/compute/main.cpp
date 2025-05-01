
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
    totalGoalAchieve = 0;
    totalPathSize = 0;
	MemoryPointers& mem = mapPtr->CPUMemory;
    for (int i = 0; i < mem.agentsCount; i++) {
		int pathSize = mem.agents[i].sizePath;
        totalGoalAchieve += (pathSize == mem.minSize_maxtimeStep_error[MINSIZE]) ? 1 : 0;
        totalPathSize += pathSize;
    }
}
const int REPEATS = 10;
int main() {
    std::string inputDir = "./saved_map/";
    std::ofstream csv("results.csv");
    csv << "File,Method,TimeRun(ms),TimeSynchronize(ms),Error\n";

    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (entry.path().extension() != ".data") continue;

        std::string file = entry.path().string();
        Map map = Map();
        
        try {
            map.load(file.c_str());


            std::vector<ComputeType> methods = {
                ComputeType::highProcesor,
                ComputeType::pureProcesorOneThread,
                ComputeType::pureProcesor,
                ComputeType::hybridGPUCPU,
                ComputeType::pureGraphicCard
            };

            for (const auto& method : methods) {
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

                csv << file << ","
                    << static_cast<int>(method) << ","
                    << (totalRun / REPEATS) << ","
                    << (totalSync / REPEATS) << ","
                    << (error.empty() ? "OK" : error) << "\n";

                std::cout << "File: " << file
                    << " | Method: " << static_cast<int>(method)
                    << " | AvgTime: " << (totalRun / REPEATS)
                    << " ms | AvgSync: " << (totalSync / REPEATS)
                    << " ms | " << (error.empty() ? "OK" : error) << "\n";
            }
        }
		catch (const std::exception& e) {
			std::cerr << "Error loading map: " << file << " - " << e.what() << "\n";
		}
    }

    csv.close();

    return 0;
}
