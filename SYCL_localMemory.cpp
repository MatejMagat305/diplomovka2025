#include <sycl/sycl.hpp>
#include <iostream>


struct Mem1{
    int *a, *b;
    int *c;
    int d, e, f;
};

inline void doCopy(const Mem1 &m, Mem1 &m2, int i){
        m2.a = &m.a[i];
        m2.b = &m.b[i];
        m2.c = &m.c[i];
        m2.d = m.d;
        m2.e = m.e;
        m2.f = m.f; 
}


int main() {
    sycl::queue queue;

    // Počet elementov
    size_t count = 15;
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
    Mem1 mC = {device_data0, device_data1, device_data2, 1, 2, 4};

    // Vytvorenie command group pre kernel
    queue.submit([&](sycl::handler& cgh) {
        Mem1 m = mC;
        cgh.parallel_for(sycl::range<1>(count), [=](sycl::id<1> idx) {
            Mem1 m2;
            int i = idx[0];
            doCopy(m, m2, i);
            *m2.a = m2.d*i*10;
            *m2.b = m2.e*i*10; 
            *m2.c = m2.f*i*10;
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
