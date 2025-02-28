#pragma once
#include <map>
#include <string>
#include "raylib.h"
#include <unordered_set>
#include <utility>
#include <vector>
#include <iostream>

#define AGENT_UNLOADER 1
#define AGENT_LOADER 2

#define LOADER_SYMBOL 'I'
#define UNLOADER_SYMBOL 'O'
#define OBSTACLE_SYMBOL '#'
#define FREEFIELD_SYMBOL '.'

// nno map symbol
#define AGENT_SYMBOL 'A'


struct Position {
    int x, y;

    // Operator < na správne porovnávanie v std::map a std::priority_queue
    bool operator<(const Position& other) const {
        return std::tie(x, y) < std::tie(other.x, other.y);
    }
};

struct Agent {
    int x = 0, y = 0, sizePath = 0;
    int unloaderCurrent;
    int loaderCurrent;
    unsigned char direction;
};

struct Constrait {
    int index, x, y;
};

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
    int g, h, f;
    Node* parent;

    Node(Position pos, int g, int h, Node* parent = nullptr)
        : pos(pos), g(g), h(h), f(g + h), parent(parent) {
    }

    bool operator>(const Node& other) const {
        return f > other.f;
    }
};

struct MemoryPointers{
    int width, height, loaderCount, unloaderCount, agentCount;
    char* grid = nullptr;
    Position* loaderPosition = nullptr;
    Position* unloaderPosition = nullptr;
    Agent* agents = nullptr;

    int* minSize = nullptr;
    Position* pathsAgent = nullptr;

    int* gCost = nullptr;
    int* fCost = nullptr;
    Position* cameFrom = nullptr;
    bool* visited = nullptr;

    Position* openList = nullptr;
    Constrait* constrait = nullptr;
    int* numberConstrait = nullptr;
};

class Map {
public:
    MemoryPointers CPUMemory;

    void save(const std::string& baseFilename);
    void load(const std::string& filename);
    void draw(int screenWidth, int screenHeight); 
    void draw2(int screenWidth, int screenHeight, int frac);

    Map(int width, int height, int agentCount, int obstacleCount, int loaderCount, int unloaderCount);
    Map();
    ~Map();
    void reset();
// private:
    int obstacleCount;
    Color* colorAgents = nullptr;
    Position selected = Position{ -1,-1 };
    bool isConnected();
    std::string getUniqueFilename(const std::string& base);
    void setNullptr();
    void saveGrid(std::stringstream* outputString);
    void saveAgents(std::stringstream* outputString);
    void saveDocks(std::stringstream* outputString);
    
    void drawGrid(int cellWidth, int cellHeight, int frac);
    void drawAgents(int cellWidth, int cellHeight, int frac);
    void drawAgentPath(const int agentIndex, int cellWidth, int cellHeight, int frac);
    void drawSelect(int cellWidth, int cellHeight, int frac);

    void loadData(const std::string& filename, std::stringstream& inputString);
    void loadDocks(std::stringstream* inputString);
    void loadAgents(std::stringstream* inputString);
    void loadGrid(std::stringstream* input_string);

    bool is_need_init();
    bool isAgentIn(int x, int y, int i);
    void initAgents();
    void initUnloader();
    void initLoader();
    void findFree(int& x, int& y);
    void initObstacle();
    void initGrid();
    void init();
    void initMem();
    bool canInitializeMemory();
    void deleteMemory();
};

// ak zvýši èas TODO MULTIPLATFORM
long long getFreeRam();

