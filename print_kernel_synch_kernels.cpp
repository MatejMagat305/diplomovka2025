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

    auto e = q.submit([&](sycl::handler& h) {
        h.parallel_for(sycl::range<1>(agentCount), [=](sycl::id<1> agentId) {
            int localMin = std::numeric_limits<int>::max();
            int startIdx = agentId[0] * segmentSize;

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
    });
    q.submit([&](sycl::handler& h) {
        h.depends_on(e);
        sycl::stream out(1024, 256, h);
        h.parallel_for(sycl::range<1>(agentCount), [=](sycl::id<1> agentId) {
            int startIdx = agentId[0] * segmentSize;
            int localMin = *globalMin;
            if (agentId[0] == 0) {
                        char buffer[16];  // Fixná veľkosť pre bezpečnosť
                        int n = localMin;
                        int i = 0;
                        bool isNegative = false;
            
                        // Spracovanie záporných čísel
                        if (n < 0) {
                            isNegative = true;
                            n = -n;
                        }
            
                        // Konverzia čísla na reťazec (v opačnom poradí)
                        do {
                            buffer[i++] = '0' + (n % 10);
                            n /= 10;
                        } while (n > 0);
            
                        if (isNegative) {
                            buffer[i++] = '-';
                        }
            
                        buffer[i] = '\0';  // Ukončovací znak
            
                        // Otočenie stringu, pretože číslo je zatiaľ v opačnom poradí
                        for (int j = 0; j < i / 2; j++) {
                            char temp = buffer[j];
                            buffer[j] = buffer[i - j - 1];
                            buffer[i - j - 1] = temp;
                        }
            
                        // Výpis na `sycl::stream`
                        out << "from kernel: " << buffer << sycl::endl;
                    }  
            

            for (int i = 0; i < segmentSize; i++) {
                d_data[startIdx + i] += localMin;
            }
        });
    }).wait();
    int result;
    int *resultArray = new int[totalSize];
    q.memcpy(&result, globalMin, sizeof(int)).wait();
    // Správna kontrola cez CPU
    int cpuMin = *std::min_element(data.begin(), data.end());

    std::cout << "Globálne minimum (GPU): " << result 
              << ", CPU kontrola: " << cpuMin << std::endl;
    q.memcpy(resultArray, d_data, totalSize * sizeof(int)).wait();
    for (int i = 0; i < totalSize; i++){
        if (data[i]+result != resultArray[i]){
            std::cout << "cpu: " << data[i]+result << " vs gpu: "<< resultArray[i] << std::endl;
            break;
        }else if (i == totalSize-1) {
            std::cout << "done" << std::endl;
        }                
    }
    
    // Uvoľnenie pamäte
    sycl::free(globalMin, q);
    sycl::free(d_data, q);
    free(resultArray);

    return 0;
}
