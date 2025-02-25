#pragma once
#include "map.h"


enum class AlgorithmType {
    ASTAR,
    // DIJKSTRA TODO later
};

enum class ComputeType {
    pureGraphicCard,
    hybridGPUCPU,
    pureProcesor,
    highProcesor
};

struct Info{
    int timeRun, timeSynchronize;
};

Info letCompute(AlgorithmType algo, ComputeType device, Map& m, int numberThread);

void checksynchronizeGPU(Map& m, Info& result);
