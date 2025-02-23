#include <sycl/sycl.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <limits>
#include <algorithm>
#include <chrono>

int main() {
    sycl::queue q{sycl::property::queue::enable_profiling{}};

    constexpr int agentCount = 1024;
    constexpr int segmentSize = 1024*1024;
    constexpr long long totalSize = agentCount * segmentSize;

    std::vector<int> data(totalSize);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(10, 150000);
    for (int &val : data) val = dist(gen);

    int* globalMin = sycl::malloc_device<int>(1, q);
    int* d_data = sycl::malloc_device<int>(totalSize, q);
    int* localMin = sycl::malloc_device<int>(agentCount, q);

    int maxInt = std::numeric_limits<int>::max();
    q.memcpy(globalMin, &maxInt, sizeof(int)).wait();
    q.memcpy(localMin, &maxInt, sizeof(int) * agentCount).wait();
    q.memcpy(d_data, data.data(), totalSize * sizeof(int)).wait();

    sycl::event event1 = q.submit([&](sycl::handler& h) {
        h.parallel_for(sycl::range<1>(agentCount), [=](sycl::id<1> agentId) {
            int id = agentId[0];
            int startIdx = id * segmentSize;
            int local_value = std::numeric_limits<int>::max();
            for (int i = 0; i < segmentSize; i++) {
                local_value = std::min(local_value, d_data[startIdx + i]);
            }
            localMin[id] = local_value;
        });
    });

    sycl::event sEvent = q.submit([&](sycl::handler& h) {
        h.depends_on(event1);
        h.single_task([=]() {
            int mini = std::numeric_limits<int>::max();
            for (size_t i = 0; i < agentCount; i++) {
                mini = std::min(mini, localMin[i]);
            }
            *globalMin = mini;
        });
    });
    sEvent.wait();

    int result1, result2;
    q.memcpy(&result1, globalMin, sizeof(int)).wait();

    auto time1 = sEvent.get_profiling_info<sycl::info::event_profiling::command_end>() -
                 event1.get_profiling_info<sycl::info::event_profiling::command_start>();

    q.memcpy(globalMin, &maxInt, sizeof(int)).wait();
    sycl::event event2 = q.submit([&](sycl::handler& h) {
        h.parallel_for(sycl::range<1>(agentCount), [=](sycl::id<1> agentId) {
            int localMin = std::numeric_limits<int>::max();
            int startIdx = agentId * segmentSize;
            for (int i = 0; i < segmentSize; i++) {
                localMin = std::min(localMin, d_data[startIdx + i]);
            }
            sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device,
                             sycl::access::address_space::global_space>
                atomicMin(*globalMin);
            atomicMin.fetch_min(localMin);
        });
    });
    event2.wait();

    q.memcpy(&result2, globalMin, sizeof(int)).wait();

    auto time2 = event2.get_profiling_info<sycl::info::event_profiling::command_end>() -
                 event2.get_profiling_info<sycl::info::event_profiling::command_start>();

    auto cpuStart = std::chrono::high_resolution_clock::now();
    int cpuMin = *std::min_element(data.begin(), data.end());
    auto cpuEnd = std::chrono::high_resolution_clock::now();
    auto cpuTime = std::chrono::duration<double, std::milli>(cpuEnd - cpuStart).count();

    std::cout << "Globálne minimum (Bez atomických operácií): " << result1 << " v " << time1 / 1.0e6 << " ms" << std::endl;
    std::cout << "Globálne minimum (S atomickými operáciami): " << result2 << " v " << time2 / 1.0e6 << " ms" << std::endl;
    std::cout << "CPU kontrola: " << cpuMin << " v " << cpuTime << " ms" << std::endl;

    sycl::free(globalMin, q);
    sycl::free(d_data, q);
    sycl::free(localMin, q);
    return 0;
}
