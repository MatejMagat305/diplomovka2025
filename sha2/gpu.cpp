#include "gpu.h"
#include <CL/sycl.hpp>
#include <iostream>
#include "sha256.h"
#include <iomanip>
using namespace cl;
using namespace sycl;

std::mutex mtx;
bool end_gpu = false;

void compute_GPU(std::atomic<bool> &found, std::atomic<std::size_t> &start_value)
{
    device device = device::get_devices(info::device_type::gpu)[0];
    queue q = queue(device);
    auto max_work_item_sizes = device.get_info<info::device::max_work_item_sizes<3>>();
    size_t compute_units = device.get_info<info::device::max_compute_units>();
    size_t total_kernels_x = max_work_item_sizes[0];
    size_t total_kernels_y = max_work_item_sizes[1];
    size_t total_kernels_z = compute_units;
    const size_t total_kernels = total_kernels_x * total_kernels_y * total_kernels_z;

    // Alokácia pamäte na GPU
    Result *results = sycl::malloc_device<Result>(total_result, q);
    bool *found_gpu = sycl::malloc_device<bool>(1, q);
    size_t *result_idx = sycl::malloc_device<size_t>(1, q);
    size_t *gpu_start_value = sycl::malloc_device<size_t>(1, q);

    q.memset(results, 0, total_result * sizeof(Result));
    q.memset(found_gpu, 0, sizeof(bool));
    q.memset(result_idx, 0, sizeof(size_t));
    end_gpu = false;
    int times = 150;

    while (!found.load())
    {
        // Aktualizácia `start_value` v GPU pamäti
        size_t local_start = start_value.fetch_add(total_kernels * times);
        q.memcpy(gpu_start_value, &local_start, sizeof(size_t)).wait();
        auto event = q.submit([&](handler &h)
                              { h.parallel_for(range<3>(total_kernels_x, total_kernels_y, total_kernels_z), [=](id<3> idx)
                                               {
                size_t global_id = idx[0] * total_kernels_y * total_kernels_z 
                + idx[1] * total_kernels_z + idx[2];
                size_t kernel_size = (total_kernels_x * total_kernels_y * total_kernels_z); 

                // Príprava dát
                size_t compute_number = global_id + *gpu_start_value;
                char input[256];

                for (int i = 0; (i < times) && !(*found_gpu); ++i) {
                    compute_number += kernel_size;

                    // Generovanie SHA-256 hashu pre každú hodnotu
                    size_t len = format_input_para(input, compute_number);
                    uint32_t w[64] = {0};
                    uint32_t hash[8] = {
                        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
                    };

                    // Naplnenie vstupného poľa pre SHA-256
                    for (size_t j = 0; j < len; ++j) {
                        w[j / 4] |= input[j] << ((3 - (j % 4)) * 8);
                    }
                    w[len / 4] |= 0x80 << ((3 - (len % 4)) * 8);
                    w[15] = len * 8;

// SHA-256 cyklus
#pragma unroll
                    for (int i = 16; i < 64; ++i) {
                        uint32_t s0 = ((w[i - 15] >> 7) | (w[i - 15] << (32 - 7))) ^
                                        ((w[i - 15] >> 18) | (w[i - 15] << (32 - 18))) ^
                                        (w[i - 15] >> 3);
                        uint32_t s1 = ((w[i - 2] >> 17) | (w[i - 2] << (32 - 17))) ^
                                        ((w[i - 2] >> 19) | (w[i - 2] << (32 - 19))) ^
                                        (w[i - 2] >> 10);
                        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
                    }

                    // Výpočet hashu
                    for (int i = 0; i < 64; ++i) {
                        uint32_t S1 = ((hash[4] >> 6) | (hash[4] << (32 - 6))) ^
                                        ((hash[4] >> 11) | (hash[4] << (32 - 11))) ^
                                        ((hash[4] >> 25) | (hash[4] << (32 - 25)));
                        uint32_t ch = (hash[4] & hash[5]) ^ (~hash[4] & hash[6]);
                        uint32_t temp1 = hash[7] + S1 + ch + K[i] + w[i];
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

                    // Kontrola výsledkov
                    if (has_leading_zeros(hash, NUM_LEADING_ZEROS)) {
                        size_t idx = sycl::atomic_ref<size_t, 
                        sycl::memory_order::relaxed, 
                        sycl::memory_scope::device>(*result_idx)
                        .fetch_add(static_cast<size_t>(1));
                        if (idx < total_result) {
                            results[idx].number = compute_number;
                            for (int j = 0; j < 8; ++j) {
                                results[idx].hash[j] = hash[j];
                            }
                            *found_gpu = true;
                            return;  // Ukonči vlákno, ak je nájdené riešenie
                        }
                    }
                } }); });
        event.wait();

        // Kopírovanie výsledkov na CPU
        bool found_host = false;
        size_t result_count = 0;
        q.memcpy(&found_host, found_gpu, sizeof(bool)).wait();
        q.memcpy(&result_count, result_idx, sizeof(size_t)).wait();

        if (found_host)
        {
            setEndGpu();
            Result *host_results = new Result[result_count];
            q.memcpy(host_results, results, result_count * sizeof(Result)).wait();

            for (size_t i = 0; i < result_count; ++i)
            {
                std::cout << "Found solution: " << host_results[i].number << " Hash: ";
                for (int j = 0; j < 8; ++j)
                {
                    std::cout << std::hex << std::setw(8) << std::setfill('0') << host_results[i].hash[j];
                }
                std::cout << std::endl;
            }
            delete[] host_results;
            found.store(true);
        }
    }
    // Uvoľnenie pamäte
    sycl::free(results, q);
    sycl::free(found_gpu, q);
    sycl::free(result_idx, q);
    sycl::free(gpu_start_value, q);
}

void setEndGpu()
{
    mtx.lock();
    end_gpu = true;
    mtx.unlock();
}
