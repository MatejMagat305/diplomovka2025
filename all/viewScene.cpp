#include "viewScene.h"

#include "mainScene.h"
#include "parameterScene.h"
#include "simulationScene.h"
#include "raygui.h"
#include "changeScene.h"

Scene* ViewScene::DrawControl() {
    map->draw(GetWindowWidth(), GetWindowHeight() - 50);

    // Buffer na text a boolean na kontrolu aktivácie textového po¾a
    static char textBoxBuffer[128] = "";  // Buffer pre text
    static bool textBoxEditMode = false;  // Boolean pre editáciu textového po¾a

    // Textové pole
    if (GuiTextBox(Rectangle{ 600, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, textBoxBuffer, 128, textBoxEditMode)) {
        textBoxEditMode = !textBoxEditMode; // Zmena režimu editácie pri kliknutí
    }

    // Tlaèidlá
    if (GuiButton(Rectangle{ 50, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Save and start")) {
        map->save(textBoxBuffer[0] != '\0' ? textBoxBuffer : "world"); // Uloženie s názvom z textového po¾a
        return StartSimulation();
    }

    if (GuiButton(Rectangle{ 160, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "New")) {
        map->reset();
        return this;
    }

    if (GuiButton(Rectangle{ 270, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Back")) {
        return new MainScene();
    }

    if (GuiButton(Rectangle{ 380, static_cast<float>(GetWindowHeight()) - 40, 130, 30 }, "Start without save")) {
        return StartSimulation();
    }

    if (GuiButton(Rectangle{ 520, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Change")) {
        Scene* result = new ChangeScene(map);
        map = nullptr;
        return result;
    }
    return this;
}

Scene* ViewScene::StartSimulation() {
    SimulationScene* result = new SimulationScene(map);
    map = nullptr;
    return result;
}

ViewScene::~ViewScene() {
    if (map != nullptr) {
        delete map;
    }
}
