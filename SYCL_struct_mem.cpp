#include <sycl/sycl.hpp>
#include <iostream>

struct Mem0{
    int *a, *b;
};

struct Mem1{
    Mem0 m;
    int *c;
};

inline void fill0(const Mem1 &m){
    for (int i = 0; i < 10; i++) {
        m.m.a[i] = 10;
    }
    for (int i = 0; i < 10; i++) {
        m.m.b[i] = 20;
    }
}

inline void fillAll(const Mem1 &m){
    for (int i = 0; i < 10; i++) {
        m.c[i] = 30;
    }
    
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
    Mem1 mC = {{device_data0, device_data1}, device_data2};

    // Vytvorenie command group pre kernel
    queue.submit([&](sycl::handler& cgh) {
        Mem1 m = mC;
        /*
        zakázané 
        for (int i = 0; i < count; i++)    {
            m.c[i] = i;
        }
        cgh.single_task([=]() {
            fillAll(m);
        });
        */
        cgh.single_task([=]() {
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
    sycl::free(device_data0, queue);
    sycl::free(device_data1, queue);
    sycl::free(device_data2, queue);
    return 0;
}
