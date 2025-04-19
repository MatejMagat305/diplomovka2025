#ifndef GPU_H
#define GPU_H

#include <atomic>

void compute_GPU(std::atomic<bool> &found, std::atomic<std::size_t> &start_value);

void setEndGpu();

#endif // GPU_H
