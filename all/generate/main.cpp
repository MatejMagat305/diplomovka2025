#include "map.h"
#include <string>
#include <vector>
#include <iostream>
#define RAYGUI_IMPLEMENTATION 
#include "raygui.h"

#include "mainScene.h"

float screenWidth = 800.;
float screenHeight = 600.;

int init(const std::string& name) {
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

int main() {
    std::vector<int> sizes = { 16, 24, 32, 64, 128, 256, 512, 1024 };
    std::vector<int> agentCounts = { 3, 4, 7, 8, 15, 16, 31, 32, 63, 64 };

    for (int size : sizes) {
        for (int agents : agentCounts) {
            // Podmienka, aby počet agentov neprevyšoval počet dostupných políčok
            if (agents >= size * size / 2) continue;

            int loaders = std::max(2, (agents / 3) * 2);
            int unloaders = loaders;

            // Prekážky: od 0 po 1/4 veľkosti mapy
            for (int fraction = 0; fraction <= 4; ++fraction) {
                int obstacleCount = (size * fraction) / 16; // 0 až 1/4 plochy
                try {
                    Map map = Map(size, size, agents, obstacleCount, loaders, unloaders);
                    std::string fileName = "map_" + std::to_string(size) + "x" + std::to_string(size) +
                        "_a" + std::to_string(agents) +
                        "_o" + std::to_string(obstacleCount) + ".map";
                    map.save(fileName.c_str());
                }
                catch (const std::exception& e) {
                    std::cerr << "Error generating map with size " << size << "x" << size
                        << ", agents: " << agents
                        << ", obstacles: " << obstacleCount << e.what() << std::endl;
                    continue;
                }

            }
        }
    }

    return 0;
}
