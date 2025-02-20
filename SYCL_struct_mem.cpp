#include <sycl/sycl.hpp>
#include <iostream>
#include <algorithm> // Pre std::fill

struct Mem0{
    int *a, *b;
};

struct Mem1{
    Mem0 m;
    int *c;
};

void fill0(Mem1 &m){
    std::fill(m.m.a, m.m.a+10, 10);
    std::fill(m.m.b, m.m.b+10, 20);
}

void fillAll(Mem1 &m){
    std::fill(m.c, m.c+10, 30);
    fill0(m);
}



int main() {
    sycl::queue queue;

    // Počet elementov
    size_t count = 10;
    size_t size = count * sizeof(int);

    // Alokácia pamäte na zariadení pomocou malloc_device
    int* device_data0 = sycl::malloc_device<int>(count, queue);
    int* device_data1 = sycl::malloc_device<int>(count, queue);
    int* device_data2 = sycl::malloc_device<int>(count, queue);

    // Kontrola alokácie
    if (device_data0 == nullptr || device_data1 == nullptr || device_data2 == nullptr) {
        std::cerr << "Chyba alokácie pamäte na zariadení!" << std::endl;
        return 1;
    }

    // Vytvorenie command group pre kernel
    queue.submit([&](sycl::handler& cgh) {
        cgh.single_task([=]() {
            Mem1 m = {{device_data0, device_data1}, device_data2};
            fillAll(m);
        });
    }).wait();

    // Kopírovanie dát späť na hostiteľa (ak je to potrebné)
    std::vector<int> host_data(count);
    queue.memcpy(host_data.data(), device_data0, size).wait();

    // Výpis dát z hostiteľa
    for (int i = 0; i < count; ++i) {
        std::cout << host_data[i] << " ";
    }
    std::cout << std::endl;
    queue.memcpy(host_data.data(), device_data1, size).wait();

    // Výpis dát z hostiteľa
    for (int i = 0; i < count; ++i) {
        std::cout << host_data[i] << " ";
    }
    std::cout << std::endl;
    queue.memcpy(host_data.data(), device_data2, size).wait();

    // Výpis dát z hostiteľa
    for (int i = 0; i < count; ++i) {
        std::cout << host_data[i] << " ";
    }
    std::cout << std::endl;

    // Uvoľnenie pamäte na zariadení
    sycl::free(device_data0, queue);
    sycl::free(device_data1, queue);
    sycl::free(device_data2, queue);
    return 0;
}
