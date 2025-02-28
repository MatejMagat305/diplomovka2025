#include "map.h"

#include <iostream>
#include <vector>
#include <utility>
#include <random>
#include <string>
#include <ctime>
#include <sstream>
#include <set>
#include <map>
#include <queue>
#include <algorithm>
#include <limits>
#include "a_star_algo.h"
#include "raylib.h"
#include "fill_init_convert.h"


std::string Map::getUniqueFilename(const std::string& base) {
    std::string filename = base;
    std::string name = std::string(GetFileNameWithoutExt(filename.c_str()));
    filename = "saved_map/" + name + ".data";
    int counter = 0;
    while (FileExists(filename.c_str())) {
        counter++;
        filename = "saved_map/" + name + std::to_string(counter) + ".data";
    }
    return filename;
}

void Map::saveGrid(std::stringstream* outputString) {
    (*outputString) << CPUMemory.width << " " << CPUMemory.height << "\n";
    for (int y = 0; y < CPUMemory.height; y++) {
        for (int x = 0; x < CPUMemory.width; x++) {
            if(!((*outputString) << CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)])){
                throw std::logic_error("Error at save grid char");
            }
        }
        if(!((*outputString) << "\n")) {
            throw std::logic_error("Error at save grid line");
        }
    }
}

void Map::saveAgents(std::stringstream* outputString) {
    (*outputString) << CPUMemory.agentCount << "\n";
    for (int i = 0; i < CPUMemory.agentCount; i++) {
        Agent &agent = CPUMemory.agents[i];
        Color& c = colorAgents[i];
        if(!((*outputString) << (int)(agent.x) << " "
            << (int)(agent.y) << " " << (int)(agent.loaderCurrent) << " "
            << (int)(agent.unloaderCurrent) << " " << (int)(agent.direction) << " "
            << (int)(c.r) << " " << (int)(c.g) << " " << (int)(c.b) << " " << (int)(c.a)
            << "\n")) {
            throw std::logic_error("Error at save agent data");
        }
        int length = CPUMemory.agents[i].sizePath;
        if (length < 0){
            length = 0;
        }
        if(!((*outputString) << length << "\n")) {
            throw std::logic_error("Error at save agent length path");
        }
        int offset = CPUMemory.width * CPUMemory.height;
        Position* path = &CPUMemory.pathsAgent[i * offset];
        for (int j = 0; j < length; j++){
            Position& p = path[j];
            if(!((*outputString) << (int)(p.x) << " " << (int)(p.y) << " ")) {
                throw std::logic_error("Error at save agent path position");
            }
        }
        if(!((*outputString) << "\n")) {
            throw std::logic_error("Error at save ending agent");
        }
    }
}

void Map::saveDocks(std::stringstream* outputString){
    if(!((*outputString) << CPUMemory.loaderCount << "\n")) {
        throw std::logic_error("error at save count of loader");
    }
    for (int i = 0; i < CPUMemory.loaderCount; i++) {
        Position& position = CPUMemory.loaderPosition[i];
        if(!((*outputString) << (int)(position.x) << " " << (int)(position.y) << " ")) {
            throw std::logic_error("error at save position of loader");
        }
    }
    if(!((*outputString) << "\n" << CPUMemory.unloaderCount << "\n")) {
        throw std::logic_error("error at save count of unloader");
    };
    for (int i = 0; i < CPUMemory.unloaderCount; i++) {
        Position& position = CPUMemory.unloaderPosition[i];
        if(!((*outputString) << (int)(position.x) << " " << (int)(position.y) << " ")) {
            throw std::logic_error("error at save position of unloader");
        }
    }
    if(!((*outputString) << "\n")) {
        throw std::logic_error("error at save ending of dock");
    }
}

void Map::save(const std::string& baseFilename) {
    if (!DirectoryExists("saved_map")) {
        MakeDirectory("saved_map");
    }
    std::string uniqueFilename = getUniqueFilename(baseFilename);
    std::stringstream outputString;
    saveGrid(&outputString);
    saveAgents(&outputString);
    saveDocks(&outputString);
    std::string protoRes = outputString.str();
    char* res = const_cast<char*>(protoRes.c_str());
    if (!SaveFileText(uniqueFilename.c_str(), res)) {
        throw std::logic_error("Failed to save file: " + uniqueFilename);
    }
}
void Map::loadGrid(std::stringstream* inputString) {
    if (!(*inputString >> CPUMemory.width >> CPUMemory.height)) {
        throw std::logic_error("Load grid error reading dimensions");
    }

    if (CPUMemory.width <= 0 || CPUMemory.height <= 0) {
        throw std::logic_error("Load grid Invalid dimensions");
    }

    CPUMemory.grid = new char[CPUMemory.height * CPUMemory.width];
    if (CPUMemory.grid == nullptr) {
        throw std::logic_error("Load grid Memory allocation failed");
    }

    for (int y = 0; y < CPUMemory.height; y++) {
        for (int x = 0; x < CPUMemory.width; x++) {
            if (!(*inputString >> CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)])) {
                delete[] CPUMemory.grid;
                CPUMemory.grid = nullptr;
                CPUMemory.width = 0;
                CPUMemory.height = 0;
                throw std::logic_error("Load grid Error reading grid data");
            }
        }
        inputString->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}


void Map::loadAgents(std::stringstream* inputString) {
    (*inputString) >> CPUMemory.agentCount;
    if (CPUMemory.agents!= nullptr) {
        delete[] CPUMemory.agents;
    }
    CPUMemory.agents = new Agent[CPUMemory.agentCount];
    if (CPUMemory.pathsAgent != nullptr)    {
        delete[] CPUMemory.pathsAgent;
    }
    CPUMemory.pathsAgent = new Position[CPUMemory.agentCount* CPUMemory.width* CPUMemory.height];
    for (int i = 0; i < CPUMemory.agentCount; i++) {
        Agent& agent = CPUMemory.agents[i];
        Color& c = colorAgents[i];
        if (!((*inputString) >> agent.x >> agent.y >> agent.loaderCurrent
            >> agent.unloaderCurrent >> agent.direction >> c.r >> c.g >> c.b >> c.a)) {
            throw std::logic_error("error at load agent");
        }

        inputString->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        int length;
        if (!((*inputString) >> length)){
            throw std::logic_error("error at load agent length path");
        }
        CPUMemory.agents[i].sizePath = length;
        int offset = CPUMemory.width * CPUMemory.height;
        Position* path = &CPUMemory.pathsAgent[i * offset];
        for (int j = 0; j < length; j++) {
            Position& p = path[j];
            if (!((*inputString) >> p.x >> p.y)) {
                throw std::logic_error("error at load position on path");
            }
        }
        inputString->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void Map::loadDocks(std::stringstream* inputString) {
    if (CPUMemory.loaderPosition != nullptr) {
        delete[] CPUMemory.loaderPosition;
    }
    if (!((*inputString) >> CPUMemory.loaderCount)){
        throw std::logic_error("error at load count of loaders");
    }

    inputString->ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    CPUMemory.loaderPosition = new Position[CPUMemory.loaderCount];

    for (int i = 0; i < CPUMemory.loaderCount; i++) {
        if (!((*inputString) >> CPUMemory.loaderPosition[i].x >> CPUMemory.loaderPosition[i].y)) {
            throw std::logic_error("error at load position of loaders");
        }
    }
    inputString->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (CPUMemory.unloaderPosition != nullptr) {
        delete[] CPUMemory.unloaderPosition;
    }
    if(!((*inputString) >> CPUMemory.unloaderCount)) {
        throw std::logic_error("error at load count of unloaders");
    }
    inputString->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    CPUMemory.unloaderPosition = new Position[CPUMemory.unloaderCount];

    for (int i = 0; i < CPUMemory.unloaderCount; i++) {
        if(!((*inputString) >> CPUMemory.unloaderPosition[i].x >> CPUMemory.unloaderPosition[i].y)) {
            throw std::logic_error("error at load position of unloaders");
        }
    }
}

void Map::loadData(const std::string& filename, std::stringstream& inputString) {
    char* data = LoadFileText(("saved_map/" + filename).data());
    if (data == nullptr) {
        std::stringstream errorString;
        errorString << "Failed to load map at " << filename << std::endl;
        TraceLog(LOG_ERROR, errorString.str().c_str());
        throw std::runtime_error(errorString.str().c_str());
    }
    inputString = std::stringstream(data);
}

// load fill Map according filename in saved_map dictionary
void Map::load(const std::string& filename) {
    std::stringstream inputString;
    loadGrid(&inputString);
    loadAgents(&inputString);
    loadDocks(&inputString);
    initMem();
}

Map::Map(int width, int height, int agentCount, int obstacleCount, int loaderCount, int unloaderCount) : obstacleCount(obstacleCount){
    CPUMemory.agentCount = agentCount;
    CPUMemory.width = width;
    CPUMemory.height = height;
    CPUMemory.loaderCount = loaderCount;
    CPUMemory.unloaderCount = unloaderCount;
    setNullptr();
	reset();
}
Map::Map() {
    obstacleCount = 0;
    setNullptr();
}

void Map::setNullptr(){
    auto agentCount = CPUMemory.agentCount;
    auto width = CPUMemory.width;
    auto  height = CPUMemory.height;
    auto loaderCount = CPUMemory.loaderCount;
    auto unloaderCount = CPUMemory.unloaderCount ;
    CPUMemory = {
        0, 0, 0, 0, 0,
        nullptr, nullptr, nullptr, 
        nullptr, nullptr, nullptr, 
        nullptr, nullptr, nullptr, 
        nullptr,  nullptr, nullptr, 
        nullptr
    };
    CPUMemory.agentCount = agentCount;
    CPUMemory.width = width;
    CPUMemory.height = height;
    CPUMemory.loaderCount = loaderCount;
    CPUMemory.unloaderCount = unloaderCount;
    colorAgents = nullptr;
}

Map::~Map() {
    deleteMemory();
}


void Map::deleteMemory() {
    delete[] CPUMemory.grid; 
    CPUMemory.grid = nullptr;
    delete[] CPUMemory.loaderPosition; 
    CPUMemory.loaderPosition = nullptr;
    delete[] CPUMemory.unloaderPosition; 
    CPUMemory.unloaderPosition = nullptr;
    delete[] CPUMemory.agents; 
    CPUMemory.agents = nullptr;
    delete[] colorAgents; 
    colorAgents = nullptr;

    delete[] CPUMemory.minSize; 
    CPUMemory.minSize = nullptr;
    delete[] CPUMemory.pathsAgent; 
    CPUMemory.pathsAgent = nullptr;

    delete[] CPUMemory.gCost; 
    CPUMemory.gCost = nullptr;
    delete[] CPUMemory.fCost; 
    CPUMemory.fCost = nullptr;
    delete[] CPUMemory.cameFrom; 
    CPUMemory.cameFrom = nullptr;
    delete[] CPUMemory.visited; 
    CPUMemory.visited = nullptr;

    delete[] CPUMemory.openList; 
    CPUMemory.openList = nullptr;
    delete[] CPUMemory.constrait; 
    CPUMemory.constrait = nullptr;
    delete[] CPUMemory.numberConstrait; 
    CPUMemory.numberConstrait = nullptr;
}


void Map::reset() {
    deleteMemory();
    srand(time(nullptr));
    while (is_need_init()) {
        init();
    }
}

bool Map::isConnected() {
    std::pair<int, int> start = std::pair<int, int>{ -1, -1 };
    for (int y = 0; y < CPUMemory.height; y++) {
        for (int x = 0; x < CPUMemory.width; x++) {
            if (CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)] != OBSTACLE_SYMBOL) {
                start = { x, y };
                break;
            }
        }
        if (start.first != -1) break;
    }
    if (start.first == -1) {
        return false;
    }
    std::vector<std::vector<bool>> visited0 = std::vector<std::vector<bool>>(CPUMemory.height, std::vector<bool>(CPUMemory.width));
    for (int i = 0; i < visited0.size(); i++) {
        for (int j = 0;j < visited0[i].size();j++) {
            visited0[i][j] = false;
        }
    }

    std::vector<std::pair<int, int>> stack;
    stack.push_back(start);
    visited0[start.second][start.first] = true;

    while (!stack.empty()) {
        auto c = stack.back();
        stack.pop_back();
        const std::vector<std::pair<int, int>> directions = {
            {0, 1}, {1, 0}, {0, -1}, {-1, 0}
        };
        for (const auto& d : directions) {
            int nx = c.first + d.first;
            int ny = c.second + d.second;
            if (nx >= 0 && nx < CPUMemory.width && ny >= 0 && ny < CPUMemory.height && CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, nx, ny)] != OBSTACLE_SYMBOL) {
                if (!visited0[ny][nx]) {
                    visited0[ny][nx] = true;
                    stack.emplace_back(nx, ny);
                }
            }
        }
    }
    for (int y = 0; y < CPUMemory.height; y++) {
        for (int x = 0; x < CPUMemory.width; x++) {
            if (CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)] != OBSTACLE_SYMBOL && !visited0[y][x]) {
                return false;
            }
        }
    }
    return true;
}

bool Map::is_need_init() {
    if (CPUMemory.grid == nullptr || CPUMemory.agents == nullptr || CPUMemory.loaderPosition == nullptr || CPUMemory.unloaderPosition == nullptr) {
        return true;
    }
    // Håbkové preh¾adávanie
    return !isConnected();
}

void Map::findFree(int& x, int& y) {
    do {
        x = rand() % CPUMemory.width;
        y = rand() % CPUMemory.height;
    } while (CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)] != '.');
}

bool Map::isAgentIn(int x, int y, int i) {
    for (int j = 0; j < i; j++){
        if (CPUMemory.agents[j].x == x && CPUMemory.agents[j].y == y) {
            return true;
        }
    }
    return false;
}

void Map::initAgents() {
    if (CPUMemory.agents != nullptr) {
        delete[] CPUMemory.agents;
    }
    CPUMemory.agents = new Agent[CPUMemory.agentCount];
    if (colorAgents != nullptr) {
        delete[] colorAgents;
    }
    colorAgents = new Color[CPUMemory.agentCount];
    if (CPUMemory.pathsAgent != nullptr){
        delete[] CPUMemory.pathsAgent;
    }
	CPUMemory.pathsAgent = new Position[CPUMemory.agentCount * CPUMemory.width * CPUMemory.height];
    for (int i = 0; i < CPUMemory.agentCount; i++) {
        int x, y;
        while (true) {
            findFree(x, y);
            if (isAgentIn(x,y,i) == false){
                break;
            }
        }
        Agent& a = CPUMemory.agents[i];
        a.x = x;
        a.y = y;
        a.direction = AGENT_LOADER;
        a.loaderCurrent = 0;
        a.unloaderCurrent = 0;
        colorAgents[i] = Color{ (unsigned char)(rand() % 256), (unsigned char)(rand() % 256), (unsigned char)(rand() % 256), 255 };
		a.sizePath = 0;
    }
}

void Map::initUnloader() {
    if (CPUMemory.unloaderPosition != nullptr) {
        delete[] CPUMemory.unloaderPosition;
    }
    CPUMemory.unloaderPosition = new Position[CPUMemory.unloaderCount];
    for (int i = 0; i < CPUMemory.unloaderCount; i++) {
        int x, y;
        findFree(x, y);
        CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)] = UNLOADER_SYMBOL;
        CPUMemory.unloaderPosition[i].x = x;
        CPUMemory.unloaderPosition[i].y = y;
    }
}

void Map::initLoader() {
    if (CPUMemory.loaderPosition != nullptr) {
        delete[] CPUMemory.loaderPosition;
    }
    CPUMemory.loaderPosition = new Position[CPUMemory.loaderCount];
    for (int i = 0; i < CPUMemory.loaderCount; i++) {
        int x, y;
        findFree(x, y);
        CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)] = LOADER_SYMBOL;
        CPUMemory.loaderPosition[i].x = x;
        CPUMemory.loaderPosition[i].y = y;
    }
}

void Map::initObstacle() {
    for (int i = 0; i < obstacleCount; i++) {
        int x, y;
        findFree(x, y);
        CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)] = OBSTACLE_SYMBOL;
    }
}

void Map::initGrid() {
    if (CPUMemory.grid != nullptr) {
        deleteMemory();
    }
    CPUMemory.grid = new char[CPUMemory.height* CPUMemory.width];
    for (int i = 0; i < CPUMemory.height* CPUMemory.width; i++) {
        CPUMemory.grid[i] = FREEFIELD_SYMBOL;
    }
}

void Map::init() {
    int gridSize = CPUMemory.height * CPUMemory.height;
    int objectNumber = CPUMemory.loaderCount + CPUMemory.unloaderCount + CPUMemory.agentCount + obstacleCount;
    if (objectNumber >= (gridSize*3)/4) {
        throw std::logic_error("overfilled map");
    }
    initMem();
    initGrid();
    initObstacle();
    initLoader();
    initUnloader();
    initAgents();
}

void Map::initMem() {
    if (!canInitializeMemory()) {
        throw std::logic_error("not enough memory");
    }

    size_t mapSize = CPUMemory.width * CPUMemory.height;
    size_t stackSize = CPUMemory.agentCount * mapSize;

    // Najskôr uvo¾níme starú pamä
    delete[] CPUMemory.minSize;
    delete[] CPUMemory.gCost;
    delete[] CPUMemory.fCost;
    delete[] CPUMemory.cameFrom;
    delete[] CPUMemory.visited;
    delete[] CPUMemory.openList;
    delete[] CPUMemory.constrait;
    delete[] CPUMemory.numberConstrait;

    try {
        CPUMemory.minSize = new int[1];
        CPUMemory.gCost = new int[stackSize];
        CPUMemory.fCost = new int[stackSize];
        CPUMemory.cameFrom = new Position[stackSize];
        CPUMemory.visited = new bool[stackSize];
        CPUMemory.openList = new Position[stackSize];
        CPUMemory.constrait = new Constrait[stackSize/4];
        CPUMemory.numberConstrait = new int[CPUMemory.agentCount];
		CPUMemory.minSize[0] = -200;
		
    }
    catch (const std::bad_alloc& e) {
        // Ak nastane chyba, dealokuj už alokovanú pamä
        delete[] CPUMemory.minSize;
        delete[] CPUMemory.gCost;
        delete[] CPUMemory.fCost;
        delete[] CPUMemory.cameFrom;
        delete[] CPUMemory.visited;
        delete[] CPUMemory.openList;
        delete[] CPUMemory.constrait;
        delete[] CPUMemory.numberConstrait; 

        throw std::runtime_error("Memory allocation failed " + std::string(e.what()));
    }
}

bool Map::canInitializeMemory() {
    size_t mapSize = CPUMemory.width * CPUMemory.height;
    size_t stackSize = CPUMemory.agentCount * mapSize;

    size_t totalAlloc = 0;
    totalAlloc += sizeof(MemoryPointers);
    totalAlloc += sizeof(int); // minSize
    totalAlloc += mapSize * sizeof(char); // grid
    totalAlloc += CPUMemory.agentCount * sizeof(Agent); // agents
    totalAlloc += CPUMemory.loaderCount * sizeof(Position); // loaderPosition
    totalAlloc += CPUMemory.unloaderCount * sizeof(Position); // unloaderPosition
    totalAlloc += stackSize * sizeof(Position); // paths
    totalAlloc += CPUMemory.agentCount * sizeof(int); // pathSizes
    totalAlloc += CPUMemory.agentCount * sizeof(int); // contraitsSizes
    totalAlloc += stackSize * sizeof(int); // gCost
    totalAlloc += stackSize * sizeof(int); // fCost
    totalAlloc += stackSize * sizeof(Position); // cameFrom
    totalAlloc += stackSize * sizeof(bool); // visited
    totalAlloc += stackSize * sizeof(Position); // openList
    totalAlloc += stackSize * sizeof(Constrait) / 4; // constrait
    totalAlloc += CPUMemory.agentCount * sizeof(int); // numberConstrait
    totalAlloc += CPUMemory.agentCount * sizeof(Color);
    size_t totalGlobalMem = getFreeRam();
    if (totalAlloc >= ((totalGlobalMem * 3) / 4)) {
        return false;
    }
    return true;
}

// ===================================== draw ====================
void Map::drawGrid(int cellWidth, int cellHeight, int frac) {
    for (int y = 0; y < CPUMemory.height; y++) {
        for (int x = 0; x < CPUMemory.width; x++) {
            Rectangle rect = { static_cast<float>(x * cellWidth), static_cast<float>(y * cellHeight),
                               static_cast<float>(cellWidth), static_cast<float>(cellHeight) };

            Color c;
            if (CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)] == OBSTACLE_SYMBOL) {
                c = BLACK;
            }
            else if (CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)] == LOADER_SYMBOL) {
                c = BLUE;
                DrawText("I", rect.x + cellWidth / 4, rect.y + cellHeight / 4, 20, DARKGRAY);
            }
            else if (CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)] == UNLOADER_SYMBOL) {
                c = YELLOW;
                DrawText("O", rect.x + cellWidth / 4, rect.y + cellHeight / 4, 20, DARKGRAY);
            }
            else {
                c = RED;
                DrawRectangleLinesEx(rect, 1, { c.r, c.g, c.b, static_cast<unsigned char>(std::max(255 / frac, 1)) });
                continue;
            }

            c.a = static_cast<unsigned char>(std::max(c.a / frac, 1));
            DrawRectangleRec(rect, c);
        }
    }
}

void Map::drawAgentPath(int agentIndex, int cellWidth, int cellHeight, int frac) {
    if (CPUMemory.agents[agentIndex].sizePath <= 0) return;

    Color c = colorAgents[agentIndex];
    Color pathColor = { c.r, c.g, c.b, static_cast<unsigned char>(std::max(60 / frac, 1)) };

    int offset = CPUMemory.width * CPUMemory.height;
    Position* path = &CPUMemory.pathsAgent[agentIndex * offset];

    for (int i = 0; i < CPUMemory.agents[agentIndex].sizePath; i++) {
        Position& pos = path[i];
        Rectangle rect = { static_cast<float>(pos.x * cellWidth), static_cast<float>(pos.y * cellHeight),
                           static_cast<float>(cellWidth), static_cast<float>(cellHeight) };
        DrawRectangleRec(rect, pathColor);
    }
}

void Map::drawAgents(int cellWidth, int cellHeight, int frac) {
    for (int i = 0; i < CPUMemory.agentCount; i++) {
        Agent& agent = CPUMemory.agents[i];
        Color& c = colorAgents[i];

        int agentX = agent.x * cellWidth + cellWidth / 2;
        int agentY = agent.y * cellHeight + cellHeight / 2;

        Color greenTransparent = { GREEN.r, GREEN.g, GREEN.b, static_cast<unsigned char>(std::max(255 / frac, 1)) };
        DrawCircle(agentX, agentY, static_cast<float>(cellWidth) / 4, greenTransparent);
        DrawCircle(agentX, agentY, static_cast<float>(cellWidth) / 8, { c.r, c.g, c.b, c.a });
    }

    for (int i = 0; i < CPUMemory.agentCount; i++) {
        drawAgentPath(i, cellWidth, cellHeight, frac);
    }
}

void Map::drawSelect(int cellWidth, int cellHeight, int frac) {
    if (selected.x == -1) return;

    Color selectedColor = { 255, 0, 0, static_cast<unsigned char>(std::max(191 / frac, 1)) };
    Rectangle selectedRect = { static_cast<float>(selected.x * cellWidth), static_cast<float>(selected.y * cellHeight),
                               static_cast<float>(cellWidth), static_cast<float>(cellHeight) };

    DrawRectangleLinesEx(selectedRect, 3, selectedColor);
}

void Map::draw(int screenWidth, int screenHeight) {
    draw2(screenWidth, screenHeight, 1);
}

void Map::draw2(int screenWidth, int screenHeight, int frac) {
    if (CPUMemory.grid == nullptr){
        return;
    }
    int cellWidth = screenWidth / CPUMemory.width;
    int cellHeight = screenHeight / CPUMemory.height;

    drawGrid(cellWidth, cellHeight, frac);
    drawAgents(cellWidth, cellHeight, frac);
    drawSelect(cellWidth, cellHeight, frac);
}

// ak zvýši èas TODO MULTIPLATFORM
long long getFreeRam() {
    return 4LL * 1024 * 1024 * 1024; // return 4 GB as standart
}