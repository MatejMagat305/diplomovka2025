#define RAYGUI_IMPLEMENTATION  // Zabezpe��, �e implement�cia sa prid� do tohto s�boru
#include "raygui.h"           // Hlavi�kov� s�bor s deklar�ciami a implement�ciou kni�nice

#include "mainScene.h"

int screenWidth = 800;
int screenHeight = 600;
int main() {
    // Inicializ�cia okna
    InitWindow(screenWidth, screenHeight, "Map Generator and Simulation");
    GuiEnable();
    // Po�iato�n� sc�na
    Scene* currentScene = new MainScene();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        // Prechod medzi sc�nami
        Scene* nextScene = currentScene->DrawControl();
        if (nextScene != currentScene) {
            delete currentScene;
            currentScene = nextScene;
        }
        EndDrawing();
    }

    // Uvo�nenie pam�te
    delete currentScene;
    CloseWindow();
    return 0;
}
