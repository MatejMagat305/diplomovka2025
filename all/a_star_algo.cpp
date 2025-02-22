#include "a_star_algo.h"
#include "fill_init_convert.h"
#include "compute.h"
#include <queue>

namespace internal {

    inline void runAStar(MemoryPointers& localMemory, MyHeap& myHeap, Position start, Position goal) {
        const int width = localMemory.width;
        const int height = localMemory.height;
        while (!empty(myHeap)) {
            Position current = pop(myHeap, localMemory.fCost, width);
            if (current.x == goal.x && current.y == goal.y) break;

            if (localMemory.visited[getTrueIndexGrid(width, current.x, current.y)]) continue;
            localMemory.visited[getTrueIndexGrid(width, current.x, current.y)] = true;

            Position neighbors[4] = {
                {current.x + 1, current.y}, {current.x - 1, current.y},
                {current.x, current.y + 1}, {current.x, current.y - 1} };

            for (int j = 0; j < 4; j++) {
                Position next = neighbors[j];
                if (next.x >= 0 && next.x < width && next.y >= 0 && next.y < height &&
                    localMemory.grid[getTrueIndexGrid(width, current.x, current.y)] != OBSTACLE_SYMBOL && !localMemory.visited[getTrueIndexGrid(width, current.x, current.y)]) {

                    int newG = localMemory.gCost[getTrueIndexGrid(width, current.x, current.y)] + 1;
                    if (newG < localMemory.gCost[getTrueIndexGrid(width, current.x, current.y)]) {
                        localMemory.gCost[getTrueIndexGrid(width, current.x, current.y)] = newG;
                        localMemory.fCost[getTrueIndexGrid(width, current.x, current.y)] = newG + myAbs(goal.x - next.x) + myAbs(goal.y - next.y);
                        localMemory.cameFrom[getTrueIndexGrid(width, current.x, current.y)] = current;
                        push(myHeap, next, localMemory.fCost, width);
                    }
                }
            }
        }
    }

    inline int reconstructPath(MemoryPointers& localMemory, Position start, Position goal) {
        const int width = localMemory.width;
        Position current = goal;
        int pathLength = 0;

        while (current.x != start.x || current.y != start.y) {
            localMemory.pathsAgent[pathLength++] = current;
            current = localMemory.cameFrom[getTrueIndexGrid(width, current.x, current.y)];
        }
        localMemory.pathsAgent[pathLength++] = start;

        // Reverse path
        for (int i = 0; i < pathLength / 2; i++) {
            Position temp = localMemory.pathsAgent[i];
            localMemory.pathsAgent[i] = localMemory.pathsAgent[pathLength - i - 1];
            localMemory.pathsAgent[pathLength - i - 1] = temp;
        }

        return pathLength;
    }

    inline void computePathForAgent(int agentId, MemoryPointers& localMemory) {
        if (localMemory.agents[agentId].sizePath != 0) {
            return;
        }
        Agent a = localMemory.agents[agentId];
        Position start = Position{a.x, a.y};
        Position goal;
        if (a.direction == AGENT_LOADER)   {
            goal = localMemory.loaderPosition[a.loaderCurrent];
        }else {
            goal = localMemory.unloaderPosition[a.unloaderCurrent];
        }
        MyHeap myHeap{ localMemory.openList, 0 };
        initializeCostsAndHeap(localMemory, myHeap, start);
        runAStar(localMemory, myHeap, start, goal);
        localMemory.agents[agentId].sizePath = reconstructPath(localMemory, start, goal);
    }
    
//====================== CPU HIGH LEVEL ===============================

    std::vector<Position> ComputeASTAR(Map& m, int agentID, const std::vector<std::vector<Constrait>>& constraints) {
        Agent& a = m.CPUMemory.agents[agentID];
        Position start = Position{ a.x, a.y };
        Position goal = (a.direction == AGENT_LOADER)
            ? m.CPUMemory.loaderPosition[a.loaderCurrent]
            : m.CPUMemory.unloaderPosition[a.unloaderCurrent];

        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
        std::map<Position, int> costSoFar;
        std::map<Position, Node*> cameFrom;

        openSet.emplace(start, 0, ManhattanHeuristic(start, goal));
        costSoFar[start] = 0;

        while (!openSet.empty()) {
            Node current = openSet.top();
            openSet.pop();

            if (isSamePosition(current.pos, goal)) {
                std::vector<Position> path;
                Node* node = &current;
                while (node) {
                    path.push_back(node->pos);
                    node = node->parent;
                }
                std::reverse(path.begin(), path.end());
                return path;
            }
            std::vector<Position> neighbors = {
                {current.pos.x + 1, current.pos.y},
                {current.pos.x - 1, current.pos.y},
                {current.pos.x, current.pos.y + 1},
                {current.pos.x, current.pos.y - 1}
            };

            for (const auto& next : neighbors) {
                if (next.x < 0 || next.y < 0 || next.x >= m.CPUMemory.width || next.y >= m.CPUMemory.height) continue;
                if (m.CPUMemory.grid[getTrueIndexGrid(m.CPUMemory.width, next.x, next.y)] == OBSTACLE_SYMBOL) continue;

                // Kontrola konfliktov v Constraint Tree
                bool conflict = false;
                auto constraintsAgent = constraints[agentID];
                for (const auto& constraint : constraintsAgent) {
                    Position actual{};
                    actual.x = constraint.x;
                    actual.y = constraint.y;
                    if (isSamePosition(actual, next)) {
                        conflict = true;
                        break;
                    }
                }
                if (conflict) continue;

                int newCost = costSoFar[current.pos] + 1;
                if (!costSoFar.count(next) || newCost < costSoFar[next]) {
                    costSoFar[next] = newCost;
                    Node* newNode = new Node(next, newCost, ManhattanHeuristic(next, goal), new Node(current.pos, current.g, current.h, current.parent));
                    cameFrom[next] = newNode;
                    openSet.push(*newNode);
                }
            }
        }

        return {}; // Ak nie je možné nájs cestu
    }

    std::vector<Position> ComputeCPUHIGHALGO(AlgorithmType which, Map& m, int agentID, const std::vector<std::vector<Constrait>>& constraints) {
        switch (which) {
        case AlgorithmType::ASTAR:
            return ComputeASTAR(m, agentID, constraints);
        default:
            return ComputeASTAR(m, agentID, constraints);
        }
    }



}