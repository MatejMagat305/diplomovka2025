#include <sycl/sycl.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <limits>
#include <algorithm>
#include <chrono>

int myAbs(int input) {
    int mask = input >> 31;  // Vytvorí -1 pre záporné čísla, 0 pre kladné
    return (input + mask) ^ mask;
}

int main() {
    sycl::queue q{ sycl::default_selector_v };

    constexpr int agentCount = 10;
    constexpr int segmentSize = 10 * 10;
    constexpr long long totalSize = agentCount * segmentSize;

    std::vector<int> data(totalSize);
    std::vector<int> dataCopy(totalSize);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(10, 150000);
    for (size_t i = 0; i < totalSize; i++){
        int val = dist(gen) - 150000;
        data[i] = val;
        dataCopy[i] = val;
    }
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
                d_data[startIdx + i] = myAbs(d_data[startIdx + i]);
            }
            });
        });
	q.memcpy(data.data(), d_data, totalSize * sizeof(int)).wait();
    for (size_t i = 0; i < totalSize; i++){
		std::cout << data[i] << " " << myAbs(dataCopy[i]) << " " << dataCopy[i] << std::endl;
    }


    sycl::free(globalMin, q);
    sycl::free(d_data, q);
    sycl::free(localMin, q);
    return 0;
}
