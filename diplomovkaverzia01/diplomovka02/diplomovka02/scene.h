#pragma once

#ifndef SCENE_H
#define SCENE_H
extern float screenWidth;
extern float screenHeight;

class Scene {
public:
    virtual Scene* DrawControl() = 0;
    virtual ~Scene() = default;

    static float GetWindowHeight() { return screenHeight; }
    static float GetWindowWidth() { return screenWidth; }
};

#endif
