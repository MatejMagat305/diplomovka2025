#pragma once
#include "map.h"
#include <sycl/sycl.hpp>

struct MyHeap {
    Position* heap;
    int size;
};

SYCL_EXTERNAL void push(MyHeap& myHeap, Position p, int* gCost, int width);
SYCL_EXTERNAL Position pop(MyHeap& myHeap, int* gCost, int width);
SYCL_EXTERNAL bool empty(MyHeap& myHeap);