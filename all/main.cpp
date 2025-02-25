#define RAYGUI_IMPLEMENTATION  // Zabezpeèí, že implementácia sa pridá do tohto súboru
#include "raygui.h"           // Hlavièkový súbor s deklaráciami a implementáciou knižnice

#include "mainScene.h"

int screenWidth = 800;
int screenHeight = 600;
int main() {
    // Inicializácia okna
    InitWindow(screenWidth, screenHeight, "Map Generator and Simulation");
    GuiEnable();
    // Poèiatoèná scéna
    Scene* currentScene = new MainScene();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        // Prechod medzi scénami
        Scene* nextScene = currentScene->DrawControl();
        if (nextScene != currentScene) {
            delete currentScene;
            currentScene = nextScene;
        }
        EndDrawing();
    }

    // Uvo¾nenie pamäte
    delete currentScene;
    CloseWindow();
    return 0;
}
