#pragma once

#ifndef SCENE_H
#define SCENE_H

static int screenWidth = 800;
static int screenHeight = 600;

class Scene {

public:
    virtual Scene* DrawControl() = 0;

    static int GetWindowHeight() {
        return screenHeight;
    }
    static int GetWindowWidth() {
        return screenWidth;
    }
};

#endif
