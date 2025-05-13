#pragma once
#include "map.h"

enum class ComputeType {
    pureGraphicCard,
    hybridGPUCPU,
    pureProcesor,
    pureProcesorOneThread,
    highProcesor
};

struct Info{
    long long timeRun, timeSynchronize;
    std::string error = "";
};

Info letCompute(ComputeType device, Map* m);
