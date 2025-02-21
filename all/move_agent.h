#pragma once
#include "agent.h"



namespace internal {
    void moveAgentForIndex(int id, Agent* agents, Position* paths, int* pathSizes, Position* loaderPosition, Position* unloaderPosition,
        int* width_height_loaderCount_unloaderCount_agentCount, int* minSize);
}