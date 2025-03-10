#include "fill_init_convert.h"    // Obsahuje pomocné funkcie, napr. getTrueIndexGrid, myAbs
#include <limits>
#include <stdlib.h>
#include "d_star_algo.h"



Key computeKey(MemoryPointers& localMemory, Position s, Position start) {
    int index = getTrueIndexGrid(localMemory.width, s.x, s.y);
    int minVal = std::min( localMemory.gCost[index], localMemory.fCost_rhs[index]);
    Key key;
    key.first = minVal + ManhattanHeuristic(s, start);
    key.second = minVal;
    return key;
}

bool keyLess(const Key& a, const Key& b) {
    return (a.first < b.first) || ((a.first == b.first) && (a.second < b.second));
}


void updateVertex_DStar(int agentID, MemoryPointers& localMemory, positionHeap& heap, Position s, Position start) {
    int index = getTrueIndexGrid(localMemory.width, s.x, s.y);

    bool blocked = (localMemory.grid[index] == OBSTACLE_SYMBOL);
    for (int i = 0; i < localMemory.numberConstraits[agentID] && !blocked; i++) {
        Constrait c = localMemory.constraits[i];
        if (isSamePosition(c.to, s) && (c.type & 0b11) == MOVE_CONSTRATINT) {
            blocked = true;
        }
    }
    if (blocked) {
        localMemory.fCost_rhs[index] = INF;
    }
    else {
        int minVal = INF;
        Position neighbors[4] = {
            {s.x + 1, s.y}, {s.x - 1, s.y},
            {s.x, s.y + 1}, {s.x, s.y - 1}
        };
        Position bestPred = { -1, -1 };
        for (int i = 0; i < 4; i++) {
            Position s2 = neighbors[i];
            if (s2.x < 0 || s2.x >= localMemory.width || s2.y < 0 || s2.y >= localMemory.height) {
				continue;
            }
            int s2Index = getTrueIndexGrid(localMemory.width, s2.x, s2.y);
            if (localMemory.grid[s2Index] == OBSTACLE_SYMBOL) {
				continue;
            }
            int cost = 1;
            int candidate = localMemory.gCost[s2Index] + cost;
            if (candidate < minVal) {
                minVal = candidate;
                bestPred = s2;
            }
        }
        localMemory.fCost_rhs[index] = minVal;
        localMemory.cameFrom[index] = bestPred;
    }
    Key key = computeKey(localMemory, s, start);
    push(heap, s, key.first);
}

void computeShortestPath_DStar(int agentID, MemoryPointers& localMemory, positionHeap& heap, Position start, Position goal) {
    while (!empty(heap)) {
        Position u = pop(heap);
        int uIndex = getTrueIndexGrid(localMemory.width, u.x, u.y);
        Key uKey = computeKey(localMemory, u, start);
        Key startCurrentKey = computeKey(localMemory, start, start);
        if (!keyLess(startCurrentKey, uKey))
            break;

        if (localMemory.gCost[uIndex] > localMemory.fCost_rhs[uIndex]) {
            localMemory.gCost[uIndex] = localMemory.fCost_rhs[uIndex];
            Position neighbors[4] = {
                {u.x + 1, u.y}, {u.x - 1, u.y},
                {u.x, u.y + 1}, {u.x, u.y - 1}
            };
            for (int i = 0; i < 4; i++) {
                Position s = neighbors[i];
                if (s.x < 0 || s.x >= localMemory.width || s.y < 0 || s.y >= localMemory.height) {
					continue;
                }
                updateVertex_DStar(agentID, localMemory, heap, s, start);
            }
        }
        else {
            localMemory.gCost[uIndex] = INF;
            updateVertex_DStar(agentID, localMemory, heap, u, start);
            Position neighbors[4] = {
                {u.x + 1, u.y}, {u.x - 1, u.y},
                {u.x, u.y + 1}, {u.x, u.y - 1}
            };
            for (int i = 0; i < 4; i++) {
                Position s = neighbors[i];
                if (s.x < 0 || s.x >= localMemory.width || s.y < 0 || s.y >= localMemory.height) {
                    continue;
                }
                updateVertex_DStar(agentID, localMemory, heap, s, start);
            }
        }
    }
}


void computePathForAgent_DStar(int agentId, MemoryPointers& localMemory, positionHeap& heap) {
    Agent a = localMemory.agents[agentId];
    Position start = { a.x, a.y };
    Position goal;
    if (a.direction == AGENT_LOADER) {
        goal = localMemory.loaderPositions[a.loaderCurrent];
    }
    else {
        goal = localMemory.unloaderPositions[a.unloaderCurrent];
    }
    int goalIndex = getTrueIndexGrid(localMemory.width, goal.x, goal.y);
    localMemory.fCost_rhs[goalIndex] = 0;
    Key goalKey = computeKey(localMemory, goal, start);
    push(heap, start, goalKey.first);
    computeShortestPath_DStar(agentId, localMemory, heap, start, goal);
    localMemory.agents[agentId].sizePath = reconstructPath(agentId, localMemory, start, goal);
}

bool isSamePosition(Position& a, Position& b){
    return bool(a.x==b.x && a.y == b.y);
}
