#include "heap_primitive.h"

SYCL_EXTERNAL void push(positionHeap& myHeap, Position p, int priority) {
    int i = myHeap.size;
    myHeap.heap[i].pos = p;
    myHeap.heap[i].priority = priority;
    myHeap.size++;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (myHeap.heap[parent].priority <= myHeap.heap[i].priority)
            break;
        HeapPositionNode temp = myHeap.heap[i];
        myHeap.heap[i] = myHeap.heap[parent];
        myHeap.heap[parent] = temp;
        i = parent;
    }
}

SYCL_EXTERNAL Position pop(positionHeap& myHeap) {
    if (myHeap.size == 0) {
        return { -1, -1 };
    }
    Position root = myHeap.heap[0].pos;
    myHeap.size--;
    myHeap.heap[0] = myHeap.heap[myHeap.size];

    int i = 0;
    while (2 * i + 1 < myHeap.size) {
        int left = 2 * i + 1, right = 2 * i + 2;
        int smallest = left;
        if (right < myHeap.size && myHeap.heap[right].priority < myHeap.heap[left].priority)
            smallest = right;
        if (myHeap.heap[i].priority <= myHeap.heap[smallest].priority)
            break;
        HeapPositionNode temp = myHeap.heap[i];
        myHeap.heap[i] = myHeap.heap[smallest];
        myHeap.heap[smallest] = temp;
        i = smallest;
    }
    return root;
}

SYCL_EXTERNAL bool empty(positionHeap& myHeap) {
    return myHeap.size == 0;
}
