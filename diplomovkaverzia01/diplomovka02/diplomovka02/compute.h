#pragma once
#include "map.h"
#include "device_type_algoritmus.h"
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "sycl/sycl.hpp"
#define MINSIZE 0
#define NUMBERCOLISION 1
#define ERROR 2
struct PositionOwner {
    Position pos1, pos2;
    int agentID1, agentID2, step;
    PositionOwner(Position p1, Position p2, int id1, int id2, int step0) : pos1(p1), pos2(p2), agentID1(id1), agentID2(id2), step(step0) {}
    bool operator==(const PositionOwner& other) const {
        return pos2.x == other.pos2.x && pos2.y == other.pos2.y
            && pos1.x == other.pos1.x && pos1.y == other.pos1.y
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
        seed ^= std::hash<int>{}(p.pos1.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.pos1.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.pos2.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.pos2.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

struct CTNode {
    std::vector<std::vector<Position>> paths;
    std::vector<std::vector<Constraint>> constraints;
    std::unordered_set<PositionOwner, PositionOwnerHash> conflicts;

    bool operator>(const CTNode& other) const {
        return (conflicts.size() > other.conflicts.size());
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

std::vector<std::vector<Position>> resolveConflictsCBS(Map* m, CTNode& root);

void synchronizeGPUFromCPU(Map* m);
void synchronizeCPUFromGPU(Map* m);
