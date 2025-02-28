#pragma once

#ifndef SCENE_H
#define SCENE_H

#pragma once

extern int screenWidth;
extern int screenHeight;

class Scene {
public:
    virtual Scene* DrawControl() = 0;
    virtual ~Scene() = default;

    static int GetWindowHeight() { return screenHeight; }
    static int GetWindowWidth() { return screenWidth; }
};

#endif
