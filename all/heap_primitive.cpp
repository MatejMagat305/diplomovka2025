#include "heap_primitive.h"

void push(MyHeap& myHeap, Position p, int* gCost, int width) {
    myHeap.heap[myHeap.size] = p;
    myHeap.size++;
    int i = myHeap.size - 1;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (gCost[myHeap.heap[parent].y * width + myHeap.heap[parent].x] <=
            gCost[myHeap.heap[i].y * width + myHeap.heap[i].x]) {
            break;
        }

        Position temp = myHeap.heap[i];
        myHeap.heap[i] = myHeap.heap[parent];
        myHeap.heap[parent] = temp;
        i = parent;
    }
}

Position pop(MyHeap& myHeap, int* gCost, int width) {
    if (myHeap.size == 0) {
        return { -1, -1 };
    }
    Position root = myHeap.heap[0];
    --(myHeap.size);
    myHeap.heap[0] = myHeap.heap[myHeap.size];

    int i = 0;
    while (2 * i + 1 < myHeap.size) {
        int left = 2 * i + 1, right = 2 * i + 2;
        int smallest = left;

        if (right < myHeap.size &&
            gCost[myHeap.heap[right].y * width + myHeap.heap[right].x] <
            gCost[myHeap.heap[left].y * width + myHeap.heap[left].x]) {
            smallest = right;
        }

        if (gCost[myHeap.heap[i].y * width + myHeap.heap[i].x] <=
            gCost[myHeap.heap[smallest].y * width + myHeap.heap[smallest].x]) {
            break;
        }
        Position temp = myHeap.heap[i];
        myHeap.heap[i] = myHeap.heap[smallest];
        myHeap.heap[smallest] = temp;
        i = smallest;
    }

    return root;
}

bool empty(MyHeap& myHeap) {
    return myHeap.size == 0;
}

