// opakovanie
    void parallelAStar(Map& m) {
        auto e = (*q).submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<1>(m.agentCount), [=](sycl::id<1> idx) {
                computePathForAgent(idx[0], width_height_loaderCount_unloaderCount_agentCount_minSize, grid, agents,
                    loaderPosition, unloaderPosition, paths, pathSizes, fCost, gCost, visited, cameFrom, openList);
                });
            });
        for (int t = 0; t < m.height * m.width; t++) {
                e = (*q).submit([&](sycl::handler& h) {
                    h.depends_on(e); // Synchronizácia s predchádzajúcimi výpočtami
                    h.parallel_for(sycl::range<1>(m.agentCount), [=](sycl::id<1> agentId) {
                        resolveAgentCollisionAtTime(agentId, t, paths, pathSizes, agents,
                            constrait, numberConstrait, grid,
                            width_height_loaderCount_unloaderCount_agentCount_minSize);
                        });
                    });
            }
        e.wait();
        // Skopírovanie výsledkov do CPU
        q->memcpy(m.agentPaths, paths, m.agentCount * 100 * sizeof(Position)).wait();
        q->memcpy(m.agentPathSizes, pathSizes, m.agentCount * sizeof(int)).wait();
    }

    void resolveAgentCollisionAtTime(int agentId, int t,
        Position* paths, int* pathSizes, Agent* agents,
        Constrait* constrait, int* numberConstrait,
        char* grid, int* width_height_loaderCount_unloaderCount_agentCount_minSize) {

        const int width = width_height_loaderCount_unloaderCount_agentCount_minSize[WIDTHS_INDEX];
        const int height = width_height_loaderCount_unloaderCount_agentCount_minSize[HEIGHTS_INDEX];
        const int mapSize = width * height;

        int offset = agentId * mapSize;
        Position* pathsLocal = &paths[offset];
        int agentSizePath = pathSizes[agentId];
        unsigned char agentDirection = agents[agentId].direction;

        if (t >= agentSizePath - 1) return;  

        Position currentPos = pathsLocal[t];
        Position nextPos = pathsLocal[t + 1];

        int conflictAgentId = -1;
        Constrait conflictFuturePos = { -1, -1 , -1 };

        if (!checkCollision(agentId, t, nextPos, conflictAgentId,
            conflictFuturePos, paths, pathSizes,
            width_height_loaderCount_unloaderCount_agentCount_minSize[AGENTS_INDEX], mapSize)) {
            return;
        }

        if (!shouldYield(agentId, conflictAgentId, agentDirection, agents[conflictAgentId].direction,
            agentSizePath, pathSizes[conflictAgentId])) {
            return;
        }

        Position foundPath[25];
        Constrait* agentConstrait = &(constrait[agentId * (width_height_loaderCount_unloaderCount_agentCount_minSize[AGENTS_INDEX] - 1)]);
        int agentConstraitSize = numberConstrait[agentId];
        agentConstrait[agentConstraitSize] = conflictFuturePos;
        numberConstrait[agentId]++;

        int foundPathSize = findAlternativePath(currentPos, pathsLocal[t + 2], foundPath, width, height, grid,
            agentConstrait, numberConstrait[agentId], t);

        if (foundPathSize > 0) {
            applyPathShift(pathsLocal, agentSizePath, t, foundPath, foundPathSize);
            pathSizes[agentId] += foundPathSize;
        }
    }
