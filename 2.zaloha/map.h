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
#define AGENT_PARK_LOADER 3
#define AGENT_PARK_UNLOADER 4

#define LOADER_SYMBOL 'I'
#define UNLOADER_SYMBOL 'O'
#define OBSTACLE_SYMBOL '#'
#define FREEFIELD_SYMBOL '.'

// nno map symbol
#define AGENT_SYMBOL 'A'

struct Position {
    int x, y;

    bool operator<(const Position& other) const;
    bool operator==(const Position& other) const;
    bool operator!=(const Position& other) const;
};

// Špecializácia hash funkcie pre `Position`
namespace std {
    template<>
    struct hash<Position> {
        size_t operator()(const Position& p) const noexcept;
    };
}


struct Agent {
    int sizePath = 0;
    int unloaderCurrent;
    int loaderCurrent;
    unsigned char direction;
    Position position, goal;
};

struct AgentFrame{
    double x = 0, y = 0;
};


#define MOVE_CONSTRATNT 1
#define BLOCK_CONSTRATNT 2
struct Constraint {
    Position position;
	int timeStep;
};

struct Conflict {
    int agentA, agentB;
    int timeStep;
};

struct HeapPositionNode {
    Position pos;
    int priority;
};

struct positionHeap {
    HeapPositionNode* heap;
    int size;
};

struct MemoryPointers{
    int width, height, loadersCount, unloadersCount, agentsCount;
    int* gCost = nullptr;
    int* fCost = nullptr;
    int* minSize_maxtimeStep_error = nullptr;

    char* grid = nullptr;

    Position* loaderPositions = nullptr;
    Position* unloaderPositions = nullptr;
    Position* agentPaths = nullptr;
    Position* cameFrom = nullptr;

    Agent* agents = nullptr;

    bool* visited = nullptr;

    HeapPositionNode* openList = nullptr;

    // int* numberConstraints = nullptr;
    // Constraint* constraints = nullptr;
};

class Map {
public:
    MemoryPointers CPUMemory;
    void save(const std::string& baseFilename);
    void load(const std::string& filename);
    void setGoals();
    void draw(float screenWidth, float screenHeight); 
    void draw2(float screenWidth, float screenHeight, int frac);
    AgentFrame* agentFrames = nullptr;

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
    
    void drawGrid(float cellWidth, float cellHeight, int frac);
    void drawAgents(float cellWidth, float cellHeight, int frac);
    void drawAgentPath(const int agentIndex, float cellWidth, float cellHeight, int frac);
    void drawSelect(float cellWidth, float cellHeight, int frac);

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
    int capacityAgents;
    int capacityLoaders;
    int capacityUnloaders;

    void AddAgent(int x, int y);
    void RemoveAgent(int x, int y);
    void AddLoader(int x, int y);
    void RemoveLoader(int x, int y);
    void AddUnloader(int x, int y);
    void RemoveUnloader(int x, int y);
	void print();
};

// ak zvýši èas TODO MULTIPLATFORM
long long getFreeRam();

std::vector<std::string> getSavedFiles(const std::string& folder, const std::string& extension);
