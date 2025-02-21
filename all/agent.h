#pragma once
#include <map>
#include <utility>
#include <vector>
#include <iostream>

#define AGENT_UNLOADER 1
#define AGENT_LOADER 2

struct Position {
    int x, y;
};

struct Agent {
    int x=0, y=0, sizePath=0;
    int unloaderCurrent;
    int loaderCurrent;
    unsigned char direction;
};
