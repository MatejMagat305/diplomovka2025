#include <iostream>
#include <thread>
#include <atomic>
#include "cpu.h"
#include "gpu.h"

int main() {
    std::atomic<bool> found(false);
    std::atomic<size_t> start_value(0);

    try {
        std::thread gpu_thread(compute_GPU, std::ref(found), std::ref(start_value));
        //std::thread cpu_thread(compute_CPU, std::ref(found), std::ref(start_value));
        gpu_thread.join();
        //cpu_thread.join();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
