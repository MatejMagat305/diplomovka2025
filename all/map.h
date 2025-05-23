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

    bool operator<(const Position& other) const;
    bool operator==(const Position& other) const;
    bool operator!=(const Position& other) const;
};

// �pecializ�cia hash funkcie pre `Position`
namespace std {
    template<>
    struct hash<Position> {
        size_t operator()(const Position& p) const noexcept;
    };
}


struct Agent {
    int x = 0, y = 0, sizePath = 0;
    int unloaderCurrent;
    int loaderCurrent;
    unsigned char direction;
};


#define MOVE_CONSTRATINT 1
struct Constrait {
    Position to;
	int type, timeStep;
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
    char* grid = nullptr;
    Position* loaderPositions = nullptr;
    Position* unloaderPositions = nullptr;
    Agent* agents = nullptr;

    int* minSize_numberColision = nullptr;
    Position* agentPaths = nullptr;

    int* gCost = nullptr;
    int* fCost_rhs = nullptr;
    Position* cameFrom = nullptr;
    bool* visited = nullptr;

    HeapPositionNode* openList = nullptr;
    Constrait* constraits = nullptr;
    int* numberConstraits = nullptr;
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

// ak zv��i �as TODO MULTIPLATFORM
long long getFreeRam();

std::vector<std::string> getSavedFiles(const std::string& folder, const std::string& extension);
