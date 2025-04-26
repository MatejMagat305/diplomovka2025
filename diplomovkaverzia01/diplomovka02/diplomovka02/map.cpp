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
#include "raylib.h"
#include "fill_init_convert.h"
#include "raygui.h"
#include <filesystem> 

namespace fs = std::filesystem;

std::vector<std::string> getSavedFiles(const std::string& folder, const std::string& extension) {
    std::vector<std::string> files;
    if (!fs::exists(folder) || !fs::is_directory(folder)) {
        std::cerr << "error: folder \"" << folder << "\" not exit!" << std::endl;
        return files;
    }
    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.is_regular_file() && entry.path().extension() == extension) {
            files.push_back(entry.path().filename().string());
        }
    }
    return files;
}

std::string Map::getUniqueFilename(const std::string& base) {
    std::string filename = base;
    std::string name = std::string(GetFileNameWithoutExt(filename.c_str()));

    auto saved = getSavedFiles("saved_map", ".data");

    int counter = 0;
    std::string uniqueFilename;
    do {
        uniqueFilename = "saved_map/" + name;
        if (counter > 0) {
            uniqueFilename += std::to_string(counter);
        }
        uniqueFilename += ".data";
        counter++;
    } while (std::find(saved.begin(), saved.end(), uniqueFilename.substr(10)) != saved.end()); // odstráni cestu

    return uniqueFilename;
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
    (*outputString) << CPUMemory.agentsCount << "\n";
    for (int i = 0; i < CPUMemory.agentsCount; i++) {
        Agent &agent = CPUMemory.agents[i];
        Color& c = colorAgents[i];
        if(!((*outputString) << (int)(agent.position.x) << " "
            << (int)(agent.position.y) << " " << (int)(agent.loaderCurrent) << " "
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
        Position* path = &CPUMemory.agentPaths[i * offset];
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
    if(!((*outputString) << CPUMemory.loadersCount << "\n")) {
        throw std::logic_error("error at save count of loader");
    }
    for (int i = 0; i < CPUMemory.loadersCount; i++) {
        Position& position = CPUMemory.loaderPositions[i];
        if(!((*outputString) << (int)(position.x) << " " << (int)(position.y) << " ")) {
            throw std::logic_error("error at save position of loader");
        }
    }
    if(!((*outputString) << "\n" << CPUMemory.unloadersCount << "\n")) {
        throw std::logic_error("error at save count of unloader");
    };
    for (int i = 0; i < CPUMemory.unloadersCount; i++) {
        Position& position = CPUMemory.unloaderPositions[i];
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
            std::cout << CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)];
        }
        std::cout << std::endl;
        inputString->ignore(INF, '\n');
    }
}


void Map::loadAgents(std::stringstream* inputString) {
    if (!((*inputString) >> CPUMemory.agentsCount)) {
        throw std::logic_error("error at load agent");
    }
    if (CPUMemory.agents!= nullptr) {
        delete[] CPUMemory.agents;
    }
    CPUMemory.agents = new Agent[CPUMemory.agentsCount];
    if (agentFrames != nullptr) {
        delete[] agentFrames;
    }
    agentFrames = new AgentFrame[CPUMemory.agentsCount];
    if (CPUMemory.agentPaths != nullptr)    {
        delete[] CPUMemory.agentPaths;
    }
    CPUMemory.agentPaths = new Position[CPUMemory.agentsCount* CPUMemory.width* CPUMemory.height];
    if (colorAgents != nullptr) {
        delete[] colorAgents;
    }
    colorAgents = new Color[CPUMemory.agentsCount];
    for (int i = 0; i < CPUMemory.agentsCount; i++) {
        Agent& agent = CPUMemory.agents[i];
        AgentFrame& frame = agentFrames[i];
        Color& c = colorAgents[i];
        if (!((*inputString) >> agent.position.x)) {
            throw std::logic_error("error at load agent");
        }
        frame.x = (double) agent.position.x;

        if (!((*inputString) >> agent.position.y)) {
            throw std::logic_error("error at load agent");
        }
        frame.y = (double)agent.position.y;

        if (!((*inputString) >> agent.loaderCurrent)) {
            throw std::logic_error("error at load agent");
        }

        if (!((*inputString) >> agent.unloaderCurrent)) {
            throw std::logic_error("error at load agent");
        }

        if (!((*inputString) >> agent.direction)) {
            throw std::logic_error("error at load agent");
        }

        int r, g, b, a;

        if (!((*inputString) >> r >> g >> b >> a)) {
            throw std::logic_error("error at load agent");
        }
        c.r = (char)r;
        c.g = (char)g;
        c.b = (char)b;
        c.a = (char)a;
        inputString->ignore(INF, '\n');
        int length;
        if (!((*inputString) >> length)){
            throw std::logic_error("error at load agent length path");
        }
        CPUMemory.agents[i].sizePath = length;
        int offset = CPUMemory.width * CPUMemory.height;
        Position* path = &CPUMemory.agentPaths[i * offset];
        for (int j = 0; j < length; j++) {
            Position& p = path[j];
            if (!((*inputString) >> p.x >> p.y)) {
                throw std::logic_error("error at load position on path");
            }
        }
        inputString->ignore(INF, '\n');
    }
}

void Map::loadDocks(std::stringstream* inputString) {
    if (!((*inputString) >> CPUMemory.loadersCount)){
        throw std::logic_error("error at load count of loaders");
    }
    if (CPUMemory.loaderPositions != nullptr) {
        delete[] CPUMemory.loaderPositions;
    }

    inputString->ignore(INF, '\n');

    CPUMemory.loaderPositions = new Position[CPUMemory.loadersCount];

    for (int i = 0; i < CPUMemory.loadersCount; i++) {
        if (!((*inputString) >> CPUMemory.loaderPositions[i].x >> CPUMemory.loaderPositions[i].y)) {
            throw std::logic_error("error at load position of loaders");
        }
    }
    inputString->ignore(INF, '\n');
    if (CPUMemory.unloaderPositions != nullptr) {
        delete[] CPUMemory.unloaderPositions;
    }
    if(!((*inputString) >> CPUMemory.unloadersCount)) {
        throw std::logic_error("error at load count of unloaders");
    }
    inputString->ignore(INF, '\n');
    CPUMemory.unloaderPositions = new Position[CPUMemory.unloadersCount];

    for (int i = 0; i < CPUMemory.unloadersCount; i++) {
        if(!((*inputString) >> CPUMemory.unloaderPositions[i].x >> CPUMemory.unloaderPositions[i].y)) {
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
    loadData(filename, inputString);
    loadGrid(&inputString);
    loadAgents(&inputString);
    loadDocks(&inputString);
    setGoals();
    initMem();
}

void Map::setGoals(){
    for (int i = 0; i < CPUMemory.agentsCount; i++) {
        Agent& agent = CPUMemory.agents[i];
        if (agent.direction == AGENT_LOADER) {
            agent.goal = CPUMemory.loaderPositions[agent.loaderCurrent];
        }
        else{
            agent.goal = CPUMemory.unloaderPositions[agent.unloaderCurrent];
        }
    }
}

Map::Map(int width, int height, int agentCount, int obstacleCount, int loaderCount, int unloaderCount) : obstacleCount(obstacleCount){
    CPUMemory.agentsCount = agentCount;
    CPUMemory.width = width;
    CPUMemory.height = height;
    CPUMemory.loadersCount = loaderCount;
    CPUMemory.unloadersCount = unloaderCount;
    setNullptr();
	reset();
}
Map::Map() {
    obstacleCount = 0;
    setNullptr();
}

void Map::setNullptr(){
    auto agentCount = CPUMemory.agentsCount;
    auto width = CPUMemory.width;
    auto  height = CPUMemory.height;
    auto loaderCount = CPUMemory.loadersCount;
    auto unloaderCount = CPUMemory.unloadersCount ;
    CPUMemory = {
        0, 0, 0, 0, 0,
        nullptr, nullptr, nullptr, 
        nullptr, nullptr, nullptr, 
        nullptr, nullptr, nullptr, 
        nullptr,  nullptr, nullptr, 
        nullptr
    };
    CPUMemory.agentsCount = agentCount;
    CPUMemory.width = width;
    CPUMemory.height = height;
    CPUMemory.loadersCount = loaderCount;
    CPUMemory.unloadersCount = unloaderCount;
    colorAgents = nullptr;
}

Map::~Map() {
    deleteMemory();
}


void Map::deleteMemory() {
    delete[] CPUMemory.grid; 
    CPUMemory.grid = nullptr;
    delete[] CPUMemory.loaderPositions; 
    CPUMemory.loaderPositions = nullptr;
    delete[] CPUMemory.unloaderPositions; 
    CPUMemory.unloaderPositions = nullptr;
    delete[] CPUMemory.agents; 
    CPUMemory.agents = nullptr;
    delete[] colorAgents; 
    colorAgents = nullptr;

    delete[] CPUMemory.minSize_numberColision_error; 
    CPUMemory.minSize_numberColision_error = nullptr;
    delete[] CPUMemory.agentPaths; 
    CPUMemory.agentPaths = nullptr;

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
    delete[] CPUMemory.constraints; 
    CPUMemory.constraints = nullptr;
    delete[] CPUMemory.numberConstraints; 
    CPUMemory.numberConstraints = nullptr;
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
    if (CPUMemory.grid == nullptr || CPUMemory.agents == nullptr || CPUMemory.loaderPositions == nullptr || CPUMemory.unloaderPositions == nullptr) {
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

bool Map::isAgentIn(int x, int y, int lenghtAgentSearch) {
    for (int j = 0; j < lenghtAgentSearch; j++){
        Agent& agent = CPUMemory.agents[j];
        if (agent.position.x == x && agent.position.y == y) {
            return true;
        }
    }
    return false;
}

void Map::initAgents() {
    if (CPUMemory.agents != nullptr) {
        delete[] CPUMemory.agents;
    }
    CPUMemory.agents = new Agent[CPUMemory.agentsCount];
    if (agentFrames != nullptr) {
        delete[] agentFrames;
    }
    agentFrames = new AgentFrame[CPUMemory.agentsCount];
    if (colorAgents != nullptr) {
        delete[] colorAgents;
    }
    colorAgents = new Color[CPUMemory.agentsCount];
    if (CPUMemory.agentPaths != nullptr){
        delete[] CPUMemory.agentPaths;
    }
	CPUMemory.agentPaths = new Position[CPUMemory.agentsCount * CPUMemory.width * CPUMemory.height];
    for (int i = 0; i < CPUMemory.agentsCount; i++) {
        int x, y;
        while (true) {
            findFree(x, y);
            if (isAgentIn(x, y, i) == false) {
                break;
            }
        }
        Agent& agent = CPUMemory.agents[i];
        AgentFrame& frame = agentFrames[i];
        agent.position = Position{ x,y };        

        frame.x = (double)agent.position.x;
        frame.y = (double)agent.position.y;
        agent.direction = AGENT_LOADER;
        agent.loaderCurrent = 0;
        agent.unloaderCurrent = 0;
        colorAgents[i] = Color{ (unsigned char)(rand() % 256), (unsigned char)(rand() % 256), (unsigned char)(rand() % 256), 255 };
		agent.sizePath = 0;
    }
}

void Map::initUnloader() {
    if (CPUMemory.unloaderPositions != nullptr) {
        delete[] CPUMemory.unloaderPositions;
    }
    CPUMemory.unloaderPositions = new Position[CPUMemory.unloadersCount];
    for (int i = 0; i < CPUMemory.unloadersCount; i++) {
        int x, y;
        findFree(x, y);
        CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)] = UNLOADER_SYMBOL;
        CPUMemory.unloaderPositions[i].x = x;
        CPUMemory.unloaderPositions[i].y = y;
    }
}

void Map::initLoader() {
    if (CPUMemory.loaderPositions != nullptr) {
        delete[] CPUMemory.loaderPositions;
    }
    CPUMemory.loaderPositions = new Position[CPUMemory.loadersCount];
    for (int i = 0; i < CPUMemory.loadersCount; i++) {
        int x, y;
        findFree(x, y);
        CPUMemory.grid[getTrueIndexGrid(CPUMemory.width, x, y)] = LOADER_SYMBOL;
        CPUMemory.loaderPositions[i].x = x;
        CPUMemory.loaderPositions[i].y = y;
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
    int objectNumber = CPUMemory.loadersCount + CPUMemory.unloadersCount + CPUMemory.agentsCount + obstacleCount;
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
    size_t stackSize = CPUMemory.agentsCount * mapSize;

    // Najskôr uvo¾níme starú pamä
    delete[] CPUMemory.minSize_numberColision_error;
    delete[] CPUMemory.gCost;
    delete[] CPUMemory.fCost;
    delete[] CPUMemory.cameFrom;
    delete[] CPUMemory.visited;
    delete[] CPUMemory.openList;
    delete[] CPUMemory.constraints;
    delete[] CPUMemory.numberConstraints;

    try {
        CPUMemory.minSize_numberColision_error = new int[3];
        CPUMemory.gCost = new int[stackSize];
        CPUMemory.fCost = new int[stackSize];
        CPUMemory.cameFrom = new Position[stackSize];
        CPUMemory.visited = new bool[stackSize];
        CPUMemory.openList = new HeapPositionNode[stackSize];
        CPUMemory.constraints = new Constraint[stackSize/4];
        CPUMemory.numberConstraints = new int[CPUMemory.agentsCount];
		CPUMemory.minSize_numberColision_error[0] = -200;
        CPUMemory.minSize_numberColision_error[1] = 0;
        CPUMemory.minSize_numberColision_error[2] = 0;
		
    }
    catch (const std::bad_alloc& e) {
        // Ak nastane chyba, dealokuj už alokovanú pamä
        delete[] CPUMemory.minSize_numberColision_error;
        delete[] CPUMemory.gCost;
        delete[] CPUMemory.fCost;
        delete[] CPUMemory.cameFrom;
        delete[] CPUMemory.visited;
        delete[] CPUMemory.openList;
        delete[] CPUMemory.constraints;
        delete[] CPUMemory.numberConstraints; 

        throw std::runtime_error("Memory allocation failed " + std::string(e.what()));
    }
}

bool Map::canInitializeMemory() {
    size_t mapSize = CPUMemory.width * CPUMemory.height;
    size_t stackSize = CPUMemory.agentsCount * mapSize;

    size_t totalAlloc = 0;
    totalAlloc += sizeof(MemoryPointers);
    totalAlloc += sizeof(int)*2; // minSize
    totalAlloc += mapSize * sizeof(char); // grid
    totalAlloc += CPUMemory.agentsCount * sizeof(Agent); // agents
    totalAlloc += CPUMemory.loadersCount * sizeof(Position); // loaderPosition
    totalAlloc += CPUMemory.unloadersCount * sizeof(Position); // unloaderPosition
    totalAlloc += stackSize * sizeof(Position); // paths
    totalAlloc += CPUMemory.agentsCount * sizeof(int); // pathSizes
    totalAlloc += CPUMemory.agentsCount * sizeof(int); // contraitsSizes
    totalAlloc += stackSize * sizeof(int); // gCost
    totalAlloc += stackSize * sizeof(int); // fCost
    totalAlloc += stackSize * sizeof(Position); // cameFrom
    totalAlloc += stackSize * sizeof(bool); // visited
    totalAlloc += stackSize * sizeof(Position); // openList
    totalAlloc += stackSize * sizeof(Constraint) / 4; // constrait
    totalAlloc += CPUMemory.agentsCount * sizeof(int); // numberConstrait
    totalAlloc += CPUMemory.agentsCount * sizeof(Color);
    size_t totalGlobalMem = getFreeRam();
    if (totalAlloc >= ((totalGlobalMem * 3) / 4)) {
        return false;
    }
    return true;
}

void Map::AddAgent(int x, int y) {
    if (CPUMemory.agentsCount >= capacityAgents) {
        capacityAgents = (capacityAgents == 0) ? 1 : capacityAgents * 2;
        Agent* newAgents = new Agent[capacityAgents];
        std::copy(CPUMemory.agents, CPUMemory.agents + CPUMemory.agentsCount, newAgents);
        delete[] CPUMemory.agents;
        CPUMemory.agents = newAgents;
    }
    Agent a = {};
    a.position = Position{ x,y };
    a.direction = AGENT_LOADER;
    a.loaderCurrent = rand() % CPUMemory.loadersCount;
    a.unloaderCurrent = rand() % CPUMemory.unloadersCount;
    CPUMemory.agents[CPUMemory.agentsCount++] = a;
}

void Map::RemoveAgent(int x, int y) {
    for (int i = 0; i < CPUMemory.agentsCount; i++) {
        if (CPUMemory.agents[i].position.x == x && CPUMemory.agents[i].position.y == y) {
            CPUMemory.agents[i] = CPUMemory.agents[--CPUMemory.agentsCount];
            return;
        }
    }
}

void Map::AddLoader(int x, int y) {
    if (CPUMemory.loadersCount >= capacityLoaders) {
        capacityLoaders = (capacityLoaders == 0) ? 1 : capacityLoaders * 2;
        Position* newLoaders = new Position[capacityLoaders];
        std::copy(CPUMemory.loaderPositions, CPUMemory.loaderPositions + CPUMemory.loadersCount, newLoaders);
        delete[] CPUMemory.loaderPositions;
        CPUMemory.loaderPositions = newLoaders;
    }
    CPUMemory.loaderPositions[CPUMemory.loadersCount++] = { x, y };
}

void Map::RemoveLoader(int x, int y) {
    for (int i = 0; i < CPUMemory.loadersCount; i++) {
        if (CPUMemory.loaderPositions[i].x == x && CPUMemory.loaderPositions[i].y == y) {
            CPUMemory.loaderPositions[i] = CPUMemory.loaderPositions[--CPUMemory.loadersCount];
            return;
        }
    }
}

void Map::AddUnloader(int x, int y) {
    if (CPUMemory.unloadersCount >= capacityUnloaders) {
        capacityUnloaders = (capacityUnloaders == 0) ? 1 : capacityUnloaders * 2;
        Position* newUnloaders = new Position[capacityUnloaders];
        std::copy(CPUMemory.unloaderPositions, CPUMemory.unloaderPositions + CPUMemory.unloadersCount, newUnloaders);
        delete[] CPUMemory.unloaderPositions;
        CPUMemory.unloaderPositions = newUnloaders;
    }
    CPUMemory.unloaderPositions[CPUMemory.unloadersCount++] = { x, y };
}

void Map::RemoveUnloader(int x, int y) {
    for (int i = 0; i <CPUMemory.unloadersCount; i++) {
        if (CPUMemory.unloaderPositions[i].x == x && CPUMemory.unloaderPositions[i].y == y) {
            CPUMemory.unloaderPositions[i] = CPUMemory.unloaderPositions[--CPUMemory.unloadersCount];
            return;
        }
    }
}

// ===================================== draw ====================
void Map::drawGrid(float cellWidth, float cellHeight, int frac) {
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

void Map::drawAgentPath(int agentIndex, float cellWidth, float cellHeight, int frac) {
    if (CPUMemory.agents[agentIndex].sizePath <= 0) return;

    Color c = colorAgents[agentIndex];
    Color pathColor = { c.r, c.g, c.b, static_cast<unsigned char>(std::max(60 / frac, 1)) };

    int offset = CPUMemory.width * CPUMemory.height;
    Position* path = &CPUMemory.agentPaths[agentIndex * offset];

    for (int i = 0; i < CPUMemory.agents[agentIndex].sizePath; i++) {
        Position& pos = path[i];
        Rectangle rect = { static_cast<float>(pos.x) * cellWidth, static_cast<float>(pos.y) * cellHeight,
                           cellWidth, cellHeight };
        DrawRectangleRec(rect, pathColor);
    }
}

void Map::drawAgents(float cellWidth, float cellHeight, int frac) {
    for (int i = 0; i < CPUMemory.agentsCount; i++) {
        AgentFrame& agent = agentFrames[i];
        Color& c = colorAgents[i];

        double agentX = agent.x * cellWidth + cellWidth / 2;
        double agentY = agent.y * cellHeight + cellHeight / 2;

        Color greenTransparent = { GREEN.r, GREEN.g, GREEN.b, static_cast<unsigned char>(std::max(255 / frac, 1)) };
        DrawCircle(agentX, agentY, cellWidth / 4, greenTransparent);
        DrawCircle(agentX, agentY, cellWidth / 8, { c.r, c.g, c.b, c.a });
    }

    for (int i = 0; i < CPUMemory.agentsCount; i++) {
        drawAgentPath(i, cellWidth, cellHeight, frac);
    }
}

void Map::drawSelect(float cellWidth, float cellHeight, int frac) {
    if (selected.x == -1) return;

    Color selectedColor = { 255, 0, 0, static_cast<unsigned char>(std::max(191 / frac, 1)) };
    Rectangle selectedRect = { static_cast<float>(selected.x) * cellWidth, static_cast<float>(selected.y) * cellHeight,
                               cellWidth, cellHeight };

    DrawRectangleLinesEx(selectedRect, 3, selectedColor);
}

void Map::draw(float screenWidth, float screenHeight) {
    draw2(screenWidth, screenHeight, 1);
}

void Map::draw2(float screenWidth, float screenHeight, int frac) {
    if (CPUMemory.grid == nullptr){
        return;
    }
    float cellWidth = screenWidth / (float) CPUMemory.width;
    float cellHeight = screenHeight / (float) CPUMemory.height;

    drawGrid(cellWidth, cellHeight, frac);
    drawAgents(cellWidth, cellHeight, frac);
    drawSelect(cellWidth, cellHeight, frac);
}

bool Position::operator<(const Position& other) const {
    return x < other.x && y < other.y;
}

bool Position::operator==(const Position& other) const {
    return x == other.x && y == other.y;
}

bool Position::operator!=(const Position& other) const {
    return !(*this == other);
}

size_t std::hash<Position>::operator()(const Position& p) const noexcept {
    return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
}

// ak zvýši èas TODO MULTIPLATFORM
long long getFreeRam() {
    return 4LL * 1024 * 1024 * 1024; // return 4 GB as standart
}