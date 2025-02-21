#include "move_agent.h"
#include "a_star_algo.h"


namespace internal {
    void moveAgentForIndex(int id, Agent* agents, Position* paths, Position* loaderPosition, Position* unloaderPosition,
        int* width_height_loaderCount_unloaderCount_agentCount, int* minSize) {
        Agent& a = agents[id];
        const int moveSize = *minSize;
        const int width = width_height_loaderCount_unloaderCount_agentCount[WIDTHS_INDEX];
        const int height = width_height_loaderCount_unloaderCount_agentCount[HEIGHTS_INDEX];
        int offset = id * width * height;
        if (moveSize > 0) {
            Position& p = paths[offset + moveSize - 1];
            a.x = p.x;
            a.y = p.y;
        }
        else {
            return;
        }

        if (a.direction == AGENT_LOADER) {
            Position& loader = loaderPosition[a.loaderCurrent];
            if (a.x == loader.x && a.y == loader.y) {
                a.direction = AGENT_UNLOADER;
                a.loaderCurrent = (a.loaderCurrent + 1 + moveSize) % width_height_loaderCount_unloaderCount_agentCount[LOADERS_INDEX];
                // TODO random
                agents[id].sizePath = 0;
                return;
            }
        }
        else {
            Position& unloader = unloaderPosition[a.unloaderCurrent];
            if (a.x == unloader.x && a.y == unloader.y) {
                a.direction = AGENT_LOADER;
                a.unloaderCurrent = (a.unloaderCurrent + 1 + moveSize) % width_height_loaderCount_unloaderCount_agentCount[UNLOADERS_INDEX];
                agents[id].sizePath = 0;
                return;
            }
        }
        Position* agentPath = &paths[offset];
        int agentSizePath = agents[id].sizePath;
        for (int i = moveSize; i < agentSizePath; i++) {
            agentPath[i - moveSize] = agentPath[i];
        }
        agents[id].sizePath = agents[id].sizePath - moveSize;
    }
}