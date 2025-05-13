#include "mainScene.h"

#include "loadScene.h"
#include "parameterScene.h"
#include "raylib.h"
#include "raygui.h"

MainScene::MainScene() = default;

MainScene::Scene* MainScene::DrawControl() {
    DrawText("Main Menu", 350, 100, 20, DARKGRAY);

    if (GuiButton(Rectangle{ 300, 200, 200, 50 }, "New Map")) {
        return new ParameterScene();

    }

    if (GuiButton(Rectangle{ 300, 300, 200, 50 }, "Load Map")) {
        return new LoadScene();

    }
    return this;
}

MainScene::~MainScene() = default;
