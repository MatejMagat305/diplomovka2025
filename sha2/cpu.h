#ifndef CPU_H
#define CPU_H

#include <atomic>

void compute_CPU(std::atomic<bool> &found, std::atomic<std::size_t> &start_value);

void setGPUFound();

#endif // CPU_H
