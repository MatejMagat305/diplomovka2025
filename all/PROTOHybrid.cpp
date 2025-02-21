
/*

void parallelAStarHybrid(Map& m) {
    int workSize = q->get_device().get_info<sycl::info::device::max_work_group_size>();
    // cesty
    auto e = (*q).submit([&](sycl::handler& h) {
        h.parallel_for(sycl::range<1>(std::min(workSize, m.agentCount)), [=](sycl::id<1> idx) {
            const int agentsize = width_height_loaderCount_unloaderCount_agentCount[AGENTS_INDEX];
            if (idx[0] > agentsize - 1) {
                return;
            }
            internal::computePathForAgentGPU(idx[0], width_height_loaderCount_unloaderCount_agentCount, grid, agents,
                loaderPosition, unloaderPosition, paths, pathSizes, fCost, gCost, visited, cameFrom, openList);
            });
        });
    e.wait_and_throw();
    // kolízie
}
*/