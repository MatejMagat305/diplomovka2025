#include "a_star_algo.h"

#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <algorithm>

#include "fill_init_convert.h"
#include "compute.h"

SYCL_EXTERNAL bool runAStarGetIsReached(MemoryPointers& localMemory, positionHeap& myHeap, Position start, Position goal, int agentId) {
    const int width = localMemory.width;
    const int height = localMemory.height;

    while (!empty(myHeap)) {
        Position current = pop(myHeap);
        if (isSamePosition(current, goal)) { 
            return true;
        }
        int index = getTrueIndexGrid(width, current.x, current.y);

        if (localMemory.visited[index]) { 
            continue;
        }
        localMemory.visited[index] = true;

        Position neighbors[4] = {
            {current.x + 1, current.y}, {current.x - 1, current.y},
            {current.x, current.y + 1}, {current.x, current.y - 1}
        };

        for (int j = 0; j < 4; j++) {
            Position next = neighbors[j];
            int nextIndex = getTrueIndexGrid(width, next.x, next.y);

            if (next.x >= 0 && next.x < width && next.y >= 0 && next.y < height &&
                localMemory.grid[nextIndex] != OBSTACLE_SYMBOL &&
                !localMemory.visited[nextIndex]) {
                int newG = localMemory.gCost[index] + 1;
                if (newG < localMemory.gCost[nextIndex]) {
                    localMemory.gCost[nextIndex] = newG;
                    int newFCost = newG + myAbs(goal.x - next.x) + myAbs(goal.y - next.y);
                    localMemory.fCost[nextIndex] = newFCost;
                    localMemory.cameFrom[nextIndex] = current;
                    push(myHeap, next, newFCost);
                }
            }
        }
    }
    return false;
}

SYCL_EXTERNAL bool astar_algorithm_get_sussces(int agentId, MemoryPointers& localMemory) {
	Agent a = localMemory.agents[agentId];
	Position start = a.position;
	Position goal = a.goal;
	positionHeap myHeap{ localMemory.openList, 0 };
	initializeCostsAndHeap(localMemory, myHeap, start);
	return runAStarGetIsReached(localMemory, myHeap, start, goal, agentId);
}
//====================== CPU HIGH LEVEL ===============================

std::vector<Position> ComputeASTAR(Map* m, int agentID) {
    MemoryPointers& CPUMemory = m->CPUMemory;
    Agent& a = CPUMemory.agents[agentID];
    Position start = a.position, goal = a.goal;
    int width = CPUMemory.width, height = CPUMemory.height;

    std::priority_queue<std::shared_ptr<Node>, std::vector<std::shared_ptr<Node>>, CompareNodes> openSet;
    std::unordered_map<Position, std::shared_ptr<Node>> cameFrom;
    std::unordered_map<Position, int> costSoFar;

    openSet.emplace(std::make_shared<Node>(start, 0, ManhattanHeuristic(start, goal)));
    costSoFar[start] = 0;

    while (!openSet.empty()) {
        auto current = openSet.top();
        openSet.pop();

        if (isSamePosition(current->pos, goal)) {
            std::vector<Position> path;
            while (current) {
                path.push_back(current->pos);
                current = current->parent;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        std::vector<Position> neighbors = {
            {current->pos.x + 1, current->pos.y},
            {current->pos.x - 1, current->pos.y},
            {current->pos.x, current->pos.y + 1},
            {current->pos.x, current->pos.y - 1}
        };

        for (auto& next : neighbors) {
            if (next.x < 0 || next.y < 0 || next.x >= width || next.y >= height) continue;
            if (CPUMemory.grid[getTrueIndexGrid(width, next.x, next.y)] == OBSTACLE_SYMBOL) continue;

            int newCost = costSoFar[current->pos] + 1;
            if (!costSoFar.count(next) || newCost < costSoFar[next]) {
                costSoFar[next] = newCost;
                auto newNode = std::make_shared<Node>(next, newCost, ManhattanHeuristic(next, goal), current);
                cameFrom[next] = newNode;
                openSet.push(newNode);
            }
        }
    }
    return {}; // Nebola nájdená žiadna cesta
}
