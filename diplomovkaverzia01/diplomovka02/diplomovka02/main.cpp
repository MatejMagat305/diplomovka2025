#define RAYGUI_IMPLEMENTATION 
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
