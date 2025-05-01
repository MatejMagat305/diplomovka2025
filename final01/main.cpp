#include "map.h"
#include <string>
#include <vector>
#include <iostream>
#define RAYGUI_IMPLEMENTATION 
#include "compute.h"
#include "raylib.h"
#include "raygui.h"

#include "mainScene.h"

float screenWidth = 800.;
float screenHeight = 600.;

int main() {
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
