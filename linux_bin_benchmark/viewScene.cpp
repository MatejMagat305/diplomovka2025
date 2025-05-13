#include "viewScene.h"
#include "mainScene.h"
#include "parameterScene.h"
#include "simulationScene.h"
#include "raygui.h"
#include "changeScene.h"

Scene* ViewScene::DrawControl() {
    map->draw(GetWindowWidth(), GetWindowHeight() - 50);

    // Textové pole
    if (GuiTextBox(Rectangle{ 650, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, textBoxBuffer, maxSize-1, textBoxEditMode)) {
        textBoxEditMode = !textBoxEditMode; // Zmena režimu editácie pri kliknutí
    }

    if (!wasLoad && GuiButton(Rectangle{ 50, static_cast<float>(GetWindowHeight()) - 40, 100, 30 }, "Save and start")) {
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
    delete[] textBoxBuffer;
}

ViewScene::ViewScene(Map* map) : map(map) {
    init("world");
}

void ViewScene::init(std::string name){
    textBoxBuffer = new char[maxSize];
    std::copy(name.cbegin(), name.cend(), textBoxBuffer);
    textBoxBuffer[name.length()] = '\0';
    curentSize = name.length()+1;
    textBoxEditMode = false;
    wasLoad = false;
}

ViewScene::ViewScene(Map* map, std::string name): map(map){
    std::string nameWithoutExt = std::string(GetFileNameWithoutExt(name.c_str()));
    init(nameWithoutExt);
    wasLoad = true;
}
