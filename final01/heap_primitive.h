#pragma once
#include "map.h"
#include <sycl/sycl.hpp>


SYCL_EXTERNAL void push(positionHeap& myHeap, Position p, int priority);
SYCL_EXTERNAL Position pop(positionHeap& myHeap);
SYCL_EXTERNAL bool empty(positionHeap& myHeap);
