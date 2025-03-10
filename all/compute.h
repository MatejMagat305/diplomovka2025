#pragma once
#include "map.h"
#include "device_type_algoritmus.h"

struct PositionOwner {
    Position pos1, pos2;
    int agentID1, agentID2;
    PositionOwner(Position p1, Position p2, int id1, int id2) : pos1(p1), pos2(p2), agentID1(id1), agentID2(id2) {}
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
    std::vector<std::vector<Constrait>> constraints;
    std::unordered_set<PositionOwner, PositionOwnerHash> conflicts;

    bool operator>(const CTNode& other) const {
        return (conflicts.size() > other.conflicts.size());
    }
};
struct Node {
    Position pos;
    double g, rhs;
    double h;
    bool operator<(const Node& other) const {
        if (g + h < other.g + other.h) {
            return true;
        }
        else if (g + h > other.g + other.h) {
            return false;
        }
        else { // g + h je rovnaké
            return g < other.g;
        }
    }
};


Info compute_cpu_primitive(AlgorithmType which, Map& m);
Info computeSYCL(AlgorithmType which, Map& m);
Info computeHYBRID(AlgorithmType which, Map& m);
Info compute_cpu_primitive_one_thread(AlgorithmType which, Map& m);
Info computeCPU(AlgorithmType which, Map& m);
std::string initializeSYCLMemory(Map& m);

void resolveConflictsCBS(AlgorithmType which, Map& m, CTNode& root, std::vector<std::vector<Position>>& solution);

void synchronizeGPUFromCPU(Map& m);
void synchronizeCPUFromGPU(Map& m);