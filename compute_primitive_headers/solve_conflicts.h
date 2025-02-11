#pragma once
#include "heap_primitive.h"
#include "a_star_algo.h"

#include <sycl/sycl.hpp>

namespace internal {
    void processAgentCollisions(sycl::nd_item<1> item, Position* paths, int* pathSizes,
        int* width_height_loaderCount_unloaderCount_agentCount_minSize, char* grid,
        Constrait* constrait, int* numberConstrait, Agent* agents);
}