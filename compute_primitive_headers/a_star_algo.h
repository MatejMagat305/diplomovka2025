#pragma once
#include "heap_primitive.h"

namespace internal {
#define WIDTHS_INDEX 0
#define HEIGHTS_INDEX 1
#define LOADERS_INDEX 2
#define UNLOADERS_INDEX 3
#define AGENTS_INDEX 4
#define MIN_INDEX 5
#define SIZE_INDEXES 6

    void computePathForAgent(int agentId, int* width_height_loaderCount_unloaderCount_agentCount_minSize,
        char* grid, Agent* agents, Position* loaderPosition, Position* unloaderPosition,
        Position* paths, int* pathSizes, int* fCost, int* gCost, bool* visited, Position* cameFrom, Position* openList);
}
