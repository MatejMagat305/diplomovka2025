#include <CL/sycl.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <limits>
#include <algorithm> // Potrebné pre std::min_element

namespace sycl = cl::sycl;

int main() {
    sycl::queue q;

    constexpr int agentCount = 10;
    constexpr int segmentSize = 500;
    constexpr int totalSize = agentCount * segmentSize;

    // Generovanie náhodných dát (10 - 150000)
    std::vector<int> data(totalSize);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(10, 150000);
    
    for (int &val : data) val = dist(gen);

    // Alokujeme pamäť na zariadení (GPU) pomocou malloc_device()
    int* globalMin = sycl::malloc_device<int>(1, q);
    int* d_data = sycl::malloc_device<int>(totalSize, q);

    // Inicializujeme hodnotu na zariadení
    int maxInt = std::numeric_limits<int>::max();
    q.memcpy(globalMin, &maxInt, sizeof(int));
    q.memcpy(d_data, data.data(), totalSize * sizeof(int)).wait();

    q.submit([&](sycl::handler& h) {
        h.parallel_for(sycl::range<1>(agentCount), [=](sycl::id<1> agentId) {
            int localMin = std::numeric_limits<int>::max();
            int startIdx = agentId * segmentSize;

            // Každý agent prechádza svojich 500 hodnôt
            for (int i = 0; i < segmentSize; i++) {
                localMin = std::min(localMin, d_data[startIdx + i]);
            }

            // Atomicky aktualizujeme globálne minimum
            sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device,
                             sycl::access::address_space::global_space>
                atomicMin(*globalMin);

            atomicMin.fetch_min(localMin);
        });
    }).wait();

    // Skopírujeme výsledok späť na hosta
    int result;
    q.memcpy(&result, globalMin, sizeof(int)).wait();

    // Správna kontrola cez CPU
    int cpuMin = *std::min_element(data.begin(), data.end());

    std::cout << "Globálne minimum (GPU): " << result 
              << ", CPU kontrola: " << cpuMin << std::endl;

    // Uvoľnenie pamäte
    sycl::free(globalMin, q);
    sycl::free(d_data, q);

    return 0;
}
