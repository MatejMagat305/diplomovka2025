#pragma once
#include "map.h"


enum class AlgorithmType {
    DSTAR,
    // DIJKSTRA TODO later
};

enum class ComputeType {
    pureGraphicCard,
    hybridGPUCPU,
    pureProcesor,
    pureProcesorOneThread,
    highProcesor
};

struct Info{
    int timeRun, timeSynchronize;
};

Info letCompute(AlgorithmType algo, ComputeType device, Map& m);
