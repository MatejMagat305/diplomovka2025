#pragma once
#include "map.h"
#include "device_type_algoritmus.h"
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "sycl/sycl.hpp"
#include <utility>

#define MINSIZE 0
#define MAXTIMESTEP 1
#define ERROR 2
struct PairHash {
    std::size_t operator()(const std::pair<int, int>& p) const noexcept {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};

struct PairEqual {
    bool operator()(const std::pair<int, int>& lhs, const std::pair<int, int>& rhs) const {
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }
};
struct PositionOwner {
    int agentID1, agentID2, step1, step2;
    PositionOwner(int id1, int id2, int step0, int step00) : agentID1(id1), agentID2(id2), step1(step0), step2(step00) {}
    bool operator==(const PositionOwner& other) const {
		return step1 == other.step1 && step2 == other.step2
            && agentID1 == other.agentID1 && agentID2 == other.agentID2;
    }
    bool operator!=(const PositionOwner& other) const {
        return !(*this == other);
    }
};

struct PositionOwnerHash {
    std::size_t operator()(const PositionOwner& p) const {
        std::size_t seed = 0;
        seed ^= std::hash<int>{}(p.agentID1) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.agentID2) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.step1) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.step2) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};


struct CBSNode {
    std::vector<std::vector<Position>> paths; 
    std::vector<Conflict> detected_conflicts;     
    int conflictCount = 0;                         
    int turn = 0;                             
    bool operator<(const CBSNode& other) const {
        if (conflictCount != other.conflictCount) return conflictCount > other.conflictCount;
        return turn > other.turn;
    }
};

struct Node {
    Position pos;
    int g, h;
    std::shared_ptr<Node> parent;

    Node(Position pos, int g, int h, std::shared_ptr<Node> parent = nullptr)
        : pos(pos), g(g), h(h), parent(parent) {
    }

    int f() const { return g + h; }
};

// Custom comparator for priority_queue (min-heap)
struct CompareNodes {
    bool operator()(const std::shared_ptr<Node>& a, const std::shared_ptr<Node>& b) const {
        return a->f() > b->f(); // Lower f-value has higher priority
    }
};
SYCL_EXTERNAL bool isSamePosition(Position& a, Position& b);

Info compute_cpu_primitive(Map* m);
Info computeSYCL(Map* m);
Info computeHYBRID(Map* m);
Info compute_cpu_primitive_one_thread(Map* m);
Info computeCPU(Map* m);
std::string initializeSYCLMemory(Map* m);
void deleteGPUMem();

CBSNode solveCBS(const std::vector<std::vector<Position>>& initialPaths);

void synchronizeGPUFromCPU(Map* m);
void synchronizeCPUFromGPU(Map* m);
