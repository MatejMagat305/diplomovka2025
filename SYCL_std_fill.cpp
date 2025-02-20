// NEPOUŽÍVAŤ - MÁLOKTORÁ INPLEMENTÁCIA


#include <sycl/sycl.hpp>
#include <iostream>
#include <algorithm> // Pre std::fill

int main() {
    sycl::queue queue;
    // Počet elementov
    size_t count = 10;
    size_t size = count * sizeof(int);
    // Alokácia pamäte na zariadení pomocou malloc_device
    int* device_data = sycl::malloc_device<int>(count, queue);
    // Kontrola alokácie
    if (device_data == nullptr) {
        std::cerr << "Chyba alokácie pamäte na zariadení!" << std::endl;
        return 1;
    }
    // Vytvorenie command group pre kernel
    queue.submit([&](sycl::handler& cgh) {
        cgh.single_task([=]() {
            std::fill(device_data, device_data+10, 42); // Použitie std::fill
        });
    }).wait();
    // Kopírovanie dát späť na hostiteľa (ak je to potrebné)
    std::vector<int> host_data(count);
    queue.memcpy(host_data.data(), device_data, size).wait();
    // Výpis dát z hostiteľa
    for (int i = 0; i < count; ++i) {
        std::cout << host_data[i] << " ";
    }
    std::cout << std::endl;
    // Uvoľnenie pamäte na zariadení
    sycl::free(device_data, queue);
    return 0;
}
