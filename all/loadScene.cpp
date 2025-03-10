#include "loadScene.h"
#include "mainScene.h"
#include "map.h"
#include "raygui.h"
#include "viewScene.h"

LoadScene::LoadScene(){
	std::vector<std::string> maps = getSavedFiles("saved_map", ".data");
	savedMaps.clear();
	int size = maps.size();
	savedMaps.reserve(size);
	for (int i = 0; i < size; i++){
		std::string& s = maps[i];
		if (s!= ""){
			savedMaps.push_back(s);
		}
	}
    scrollPanelBounds = { 0, 0, (float)screenWidth, (float)screenHeight - 50 };
    contentBounds = { 0, 0, (float)screenWidth - 20, (float)(savedMaps.size() * 30) }; 
    backButtonBounds = { 10, (float)screenHeight - 40, 100, 30 };
    loadButtonBounds = { (float)screenWidth - 110, (float)screenHeight - 40, 100, 30 };
    view = { 0, 0, scrollPanelBounds.width, scrollPanelBounds.height };
}

Scene* LoadScene::DrawControl() {
    GuiScrollPanel(scrollPanelBounds, "select", contentBounds, &scrollOffset, &view);
    BeginScissorMode((int)scrollPanelBounds.x, (int)scrollPanelBounds.y,
        (int)scrollPanelBounds.width, (int)scrollPanelBounds.height);

    for (size_t i = 0; i < savedMaps.size(); i++) {
        Rectangle mapNameBounds = {
            scrollPanelBounds.x + 10,
            scrollPanelBounds.y + 50 + (float)(i * 30) - scrollOffset.y,
            scrollPanelBounds.width - 20, 25
        };

        if (CheckCollisionPointRec(GetMousePosition(), mapNameBounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            selectedIndex = i;
        }
        if (i == selectedIndex) {
            DrawRectangleRec(mapNameBounds, LIGHTGRAY);
        }
        DrawText(savedMaps[i].c_str(), (int)mapNameBounds.x, (int)mapNameBounds.y, 20, DARKGRAY);
    }

    EndScissorMode();
    if (GuiButton(backButtonBounds, "Back")) {
        return new MainScene();
    }
    if (GuiButton(loadButtonBounds, "Load")) {
        if (selectedIndex >= 0 && selectedIndex < savedMaps.size()) {
            Map* m = new Map();
            m->load(savedMaps[selectedIndex]);
            return new ViewScene(m);
        }
    }
    return this;
}