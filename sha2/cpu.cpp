#include "vector"
#include "thread"
#include "sha256.h"
#include "cpu.h"
#include <iostream>
#include <iomanip>

// CPU verzia funkcie na výpočet
void compute_CPU(std::atomic<bool>& found, std::atomic<std::size_t>& start_value) {
    const size_t how_many = 4096 * 4096; // Počet hodnot, ktoré každé vlákno spracuje
    const size_t total_threads = std::thread::hardware_concurrency(); // Počet vlákien, ktoré môžeme využiť

    // Atomická verzia výsledkového poľa
    std::atomic<Result> results[1];
    std::atomic<size_t> result_idx(0);
    bool cpu_found = false;

    // Lambda funkcia, ktorá bude vykonávať výpočty na CPU
    auto worker = [&](size_t thread_idx) {
        while (!found.load()) {
            size_t local_start = start_value.fetch_add(how_many);
            size_t local_end = local_start + how_many - 1;


            for (size_t current_idx = local_start; current_idx <= local_end; ++current_idx) {
                if (found.load()) return;
            char input[256] = "para";

                // Formátovanie vstupu
                size_t i = format_input_para(input, current_idx);

                // Výpočet SHA-256
                uint32_t w[64] = {0};
                uint32_t hash[8] = {
                    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
                };

                for (int j = 0; j < i; ++j) {
                    w[j / 4] |= input[j] << ((3 - (j % 4)) * 8);
                }

                w[i / 4] |= 0x80 << ((3 - (i % 4)) * 8);
                w[15] = i * 8;

                // SHA-256 výpočet
                for (int j = 16; j < 64; ++j) {
                    uint32_t s0 = ((w[j - 15] >> 7) | (w[j - 15] << (32 - 7))) ^
                                    ((w[j - 15] >> 18) | (w[j - 15] << (32 - 18))) ^
                                    (w[j - 15] >> 3);
                    uint32_t s1 = ((w[j - 2] >> 17) | (w[j - 2] << (32 - 17))) ^
                                    ((w[j - 2] >> 19) | (w[j - 2] << (32 - 19))) ^
                                    (w[j - 2] >> 10);
                    w[j] = w[j - 16] + s0 + w[j - 7] + s1;
                }

                for (int j = 0; j < 64; ++j) {
                    uint32_t S1 = ((hash[4] >> 6) | (hash[4] << (32 - 6))) ^
                                    ((hash[4] >> 11) | (hash[4] << (32 - 11))) ^
                                    ((hash[4] >> 25) | (hash[4] << (32 - 25)));
                    uint32_t ch = (hash[4] & hash[5]) ^ (~hash[4] & hash[6]);
                    uint32_t temp1 = hash[7] + S1 + ch + w[j];
                    uint32_t S0 = ((hash[0] >> 2) | (hash[0] << (32 - 2))) ^
                                    ((hash[0] >> 13) | (hash[0] << (32 - 13))) ^
                                    ((hash[0] >> 22) | (hash[0] << (32 - 22)));
                    uint32_t maj = (hash[0] & hash[1]) ^ (hash[0] & hash[2]) ^ (hash[1] & hash[2]);
                    uint32_t temp2 = S0 + maj;

                    hash[7] = hash[6];
                    hash[6] = hash[5];
                    hash[5] = hash[4];
                    hash[4] = hash[3] + temp1;
                    hash[3] = hash[2];
                    hash[2] = hash[1];
                    hash[1] = hash[0];
                    hash[0] = temp1 + temp2;
                }

                // Kontrola, či hash začína požadovanými nulami
                if (has_leading_zeros(hash, NUM_LEADING_ZEROS)) {
                    size_t idx = result_idx.fetch_add(1);
                    if (idx < 1) {
                        Result result;
                        result.number = current_idx;
                        for (int j = 0; j < 8; ++j) {
                            result.hash[j] = hash[j];
                        }

                        results[idx].store(result);  // Atomické uloženie výsledku
                        found.store(true);
                        cpu_found = true;
                    }
                }
            }
        }
    };

    // Vytvárame a spúšťame vlákna
    std::vector<std::thread> threads;
    for (size_t i = 0; i < total_threads; ++i) {
        threads.push_back(std::thread(worker, i));
    }

    // Čakanie na dokončenie všetkých vlákien
    for (auto& t : threads) {
        t.join();
    }

    // Zobrazenie výsledkov
    if (cpu_found) {
        for (size_t i = 0; i < result_idx.load(); ++i) {
            Result result = results[i].load();  // Načítanie atomického výsledku
            std::cout << "Found solution: " << result.number << " Hash: ";
            for (int j = 0; j < 8; ++j) {
                std::cout << std::hex << std::setw(8) << std::setfill('0') << result.hash[j];
            }
            std::cout << std::endl;
        }
    }
}

