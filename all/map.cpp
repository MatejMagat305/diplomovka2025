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
#include "agent.h"


void Map::saveGrid(std::stringstream* outputString) {
    (*outputString) << width << " " << height << "\n";
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if(!((*outputString) << m.grid[getTrueIndexGrid(width, x, y)])){
                throw std::logic_error("Error at save grid char");
            }
        }
        if(!((*outputString) << "\n")) {
            throw std::logic_error("Error at save grid line");
        }
    }
}

void Map::saveAgents(std::stringstream* outputString) {
    (*outputString) << agentCount << "\n";
    for (int i = 0; i < agentCount; i++) {
        Agent &agent = m.agents[i];
        Color& c = colorAgents[i];
        if(!((*outputString) << agent.x << " " 
            << agent.y << " " << agent.loaderCurrent << " "
            << agent.unloaderCurrent << " " << agent.direction << " " 
            << c.r << " " << c.g << " " << c.b << c.a 
            << "\n")) {
            throw std::logic_error("Error at save agent data");
        }
        int length = m.pathSizesAgent[i];
        if (length < 0){
            length = 0;
        }
        if(!((*outputString) << length << "\n")) {
            throw std::logic_error("Error at save agent length path");
        }
        for (int j = 0; j < length; j++){
            Position& p = m.pathsAgent[getTrueIndexPath(i, j)];
            if(!((*outputString) << p.x << " " << p.y << " ")) {
                throw std::logic_error("Error at save agent path position");
            }
        }
        if(!((*outputString) << "\n")) {
            throw std::logic_error("Error at save ending agent");
        }
    }
}

void Map::saveDocks(std::stringstream* outputString){
    if(!((*outputString) << loaderCount << "\n")) {
        throw std::logic_error("error at save count of loader");
    }
    for (int i = 0; i < loaderCount; i++) {
        Position& position = m.loaderPosition[i];
        if(!((*outputString) << position.x << position.y << " ")) {
            throw std::logic_error("error at save position of loader");
        }
    }
    if(!((*outputString) << "\n" << unloaderCount << "\n")) {
        throw std::logic_error("error at save count of unloader");
    };
    for (int i = 0; i < unloaderCount; i++) {
        Position& position = m.unloaderPosition[i];
        if(!((*outputString) << position.x << position.y << " ")) {
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
    std::string uniqueFilename = getUniqueFilename("saved_map/" + baseFilename);
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
    if (!(*inputString >> width >> height)) {
        throw std::logic_error("Load grid error reading dimensions");
    }

    if (width <= 0 || height <= 0) {
        throw std::logic_error("Load grid Invalid dimensions");
    }

    m.grid = new char[height * width];
    if (m.grid == nullptr) {
        throw std::logic_error("Load grid Memory allocation failed");
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (!(*inputString >> m.grid[getTrueIndexGrid(width, x, y)])) {
                delete[] m.grid;
                m.grid = nullptr;
                width = 0;
                height = 0;
                throw std::logic_error("Load grid Error reading grid data");
            }
        }
        inputString->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}


void Map::loadAgents(std::stringstream* inputString) {
    (*inputString) >> agentCount;
    if (m.agents!= nullptr) {
        delete[] m.agents;
    }
    m.agents = new Agent[agentCount];
    if (m.pathsAgent != nullptr)    {
        delete[] m.pathsAgent;
    }
    m.pathsAgent = new Position[agentCount*width*height];
    if (m.pathSizesAgent != nullptr) {
        delete[] m.pathSizesAgent;
    }
    m.pathSizesAgent = new int[agentCount];
    for (int i = 0; i < agentCount; i++) {
        Agent& agent = m.agents[i];
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
        m.pathSizesAgent[i] = length;
        for (int j = 0; j < length; j++) {
            Position& p = m.pathsAgent[getTrueIndexPath(i, j)];
            if (!((*inputString) >> p.x >> p.y)) {
                throw std::logic_error("error at load position on path");
            }
        }
        inputString->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void Map::loadDocks(std::stringstream* inputString) {
    if (m.loaderPosition != nullptr) {
        delete[] m.loaderPosition;
    }
    if (!((*inputString) >> loaderCount)){
        throw std::logic_error("error at load count of loaders");
    }

    inputString->ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    m.loaderPosition = new Position[loaderCount];

    for (int i = 0; i < loaderCount; i++) {
        if (!((*inputString) >> m.loaderPosition[i].x >> m.loaderPosition[i].y)) {
            throw std::logic_error("error at load position of loaders");
        }
    }
    inputString->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (m.unloaderPosition != nullptr) {
        delete[] m.unloaderPosition;
    }
    if(!((*inputString) >> unloaderCount)) {
        throw std::logic_error("error at load count of unloaders");
    }
    inputString->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    m.unloaderPosition = new Position[unloaderCount];

    for (int i = 0; i < unloaderCount; i++) {
        if(!((*inputString) >> m.unloaderPosition[i].x >> m.unloaderPosition[i].y)) {
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

Map::Map(int width, int height, int agentCount, int obstacleCount, int loaderCount, int unloaderCount) : width(width), height(height), 
loaderCount(loaderCount), unloaderCount(unloaderCount), obstacleCount(obstacleCount), agentCount(agentCount){
    setNullptr();
    reset();
}
Map::Map() {
    width = 0; height = 0; loaderCount = 0; unloaderCount = 0;
    setNullptr();
}

void Map::setNullptr(){
    m = {
        nullptr, nullptr, nullptr, 
        nullptr, nullptr, nullptr, 
        nullptr, nullptr, nullptr, 
        nullptr,  nullptr, nullptr, 
        nullptr, nullptr, nullptr, 
        nullptr
    };
    colorAgents = nullptr;
}

Map::~Map() {
    deleteMemory();
}

void Map::deleteMemory() {
    delete[] m.grid;
    delete[] m.loaderPosition;
    delete[] m.unloaderPosition;
    delete[] m.agents;
    delete[] colorAgents;

    delete[] m.stuffWHLUA;
    delete[] m.minSize;
    delete[] m.agents;
    delete[] m.pathsAgent;
    delete[] m.pathSizesAgent;

    delete[] m.gCost;
    delete[] m.fCost;
    delete[] m.cameFrom;
    delete[] m.visited;

    delete[] m.openList;
    delete[] m.constrait;
    delete[] m.numberConstrait;
}

void Map::reset() {
    deleteMemory();
    setNullptr();
    srand(time(nullptr));
    while (is_need_init()) {
        init();
    }
}

std::string Map::getUniqueFilename(const std::string& base) {
    std::string filename = base;
    std::string name = std::string(GetFileNameWithoutExt(filename.c_str()));
    int counter = 0;
    while (FileExists(filename.c_str())) {
        counter++;
        filename = name + std::to_string(counter) + ".txt";
    }
    return filename;
}
bool Map::isConnected() {
    std::pair<int, int> start = std::pair<int, int>{ -1, -1 };
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (m.grid[getTrueIndexGrid(width, x, y)] != OBSTACLE_SYMBOL) {
                start = { x, y };
                break;
            }
        }
        if (start.first != -1) break;
    }
    if (start.first == -1) {
        return false;
    }
    std::vector<std::vector<bool>> visited0 = std::vector<std::vector<bool>>(height, std::vector<bool>(width));

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
            if (nx >= 0 && nx < width && ny >= 0 && ny < height && m.grid[getTrueIndexGrid(width, nx, ny)] != OBSTACLE_SYMBOL) {
                if (!visited0[ny][nx]) {
                    visited0[ny][nx] = true;
                    stack.emplace_back(nx, ny);
                }
            }
        }
    }
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (m.grid[getTrueIndexGrid(width, x, y)] != OBSTACLE_SYMBOL && !visited0[y][x]) {
                return false;
            }
        }
    }
    return true;
}

bool Map::is_need_init() {
    if (m.grid == nullptr || m.agents == nullptr || m.loaderPosition == nullptr || m.unloaderPosition == nullptr) {
        return true;
    }
    // Håbkové preh¾adávanie
    return !isConnected();
}

void Map::findFree(int& x, int& y) {
    do {
        x = rand() % width;
        y = rand() % height;
    } while (m.grid[getTrueIndexGrid(width, x, y)] != '.');
}

bool Map::isAgentIn(int x, int y, int i) {
    for (int j = 0; j < i; j++){
        if (m.agents[j].x == x && m.agents[j].y == y) {
            return true;
        }
    }
    return false;
}

void Map::initAgents() {
    if (m.agents != nullptr) {
        delete[] m.agents;
    }
    m.agents = new Agent[agentCount];
    if (colorAgents != nullptr) {
        delete[] colorAgents;
    }
    colorAgents = new Color[agentCount];
    for (int i = 0; i < agentCount; i++) {
        int x, y;
        while (true) {
            findFree(x, y);
            if (isAgentIn(x,y,i) == false){
                break;
            }
        }
        Agent& a = m.agents[i];
        a.x = x;
        a.y = y;
        a.direction = AGENT_LOADER;
        a.loaderCurrent = 0;
        a.unloaderCurrent = 0;
        colorAgents[i] = Color{ (unsigned char)(rand() % 256), (unsigned char)(rand() % 256), (unsigned char)(rand() % 256), 255 };
    }
}

void Map::initUnloader() {
    if (m.unloaderPosition != nullptr) {
        delete[] m.unloaderPosition;
    }
    m.unloaderPosition = new Position[unloaderCount];
    for (int i = 0; i < unloaderCount; i++) {
        int x, y;
        findFree(x, y);
        m.grid[getTrueIndexGrid(width, x, y)] = UNLOADER_SYMBOL;
        m.unloaderPosition[i].x = x;
        m.unloaderPosition[i].y = y;
    }
}

void Map::initLoader() {
    if (m.loaderPosition != nullptr) {
        delete[] m.loaderPosition;
    }
    m.loaderPosition = new Position[loaderCount];
    for (int i = 0; i < loaderCount; i++) {
        int x, y;
        findFree(x, y);
        m.grid[getTrueIndexGrid(width, x, y)] = LOADER_SYMBOL;
        m.loaderPosition[i].x = x;
        m.loaderPosition[i].y = y;
    }
}

void Map::initObstacle() {
    for (int i = 0; i < obstacleCount; i++) {
        int x, y;
        findFree(x, y);
        m.grid[getTrueIndexGrid(width, x, y)] = OBSTACLE_SYMBOL;
    }
}

void Map::initGrid() {
    if (m.grid != nullptr) {
        deleteMemory();
    }
    m.grid = new char[height*width];
    for (int i = 0; i < height*width; i++) {
        m.grid[i] = FREEFIELD_SYMBOL;
    }
}

void Map::init() {
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

    size_t mapSize = width * height;
    size_t stackSize = agentCount * mapSize;

    // Najskôr uvo¾níme starú pamä
    delete[] m.stuffWHLUA;
    delete[] m.minSize;
    delete[] m.gCost;
    delete[] m.fCost;
    delete[] m.cameFrom;
    delete[] m.visited;
    delete[] m.openList;
    delete[] m.constrait;
    delete[] m.numberConstrait;

    try {
        m.stuffWHLUA = new int[SIZE_INDEXES];
        m.minSize = new int[1];
        m.gCost = new int[stackSize];
        m.fCost = new int[stackSize];
        m.cameFrom = new Position[stackSize];
        m.visited = new bool[stackSize];
        m.openList = new Position[stackSize];
        m.constrait = new Constrait[agentCount * (agentCount - 1)];
        m.numberConstrait = new int[agentCount];
    }
    catch (const std::bad_alloc& e) {
        // Ak nastane chyba, dealokuj už alokovanú pamä
        delete[] m.stuffWHLUA;
        delete[] m.minSize;
        delete[] m.gCost;
        delete[] m.fCost;
        delete[] m.cameFrom;
        delete[] m.visited;
        delete[] m.openList;
        delete[] m.constrait;
        delete[] m.numberConstrait; 

        throw std::runtime_error("Memory allocation failed " + std::string(e.what()));
    }
}

bool Map::canInitializeMemory() {
    size_t mapSize = width * height;
    size_t stackSize = agentCount * mapSize;

    size_t totalAlloc = 0;
    totalAlloc += 6 * sizeof(int); // width_height_loaderCount_unloaderCount_agentCount + minSize
    totalAlloc += mapSize * sizeof(char); // grid
    totalAlloc += agentCount * sizeof(Agent); // agents
    totalAlloc += loaderCount * sizeof(Position); // loaderPosition
    totalAlloc += unloaderCount * sizeof(Position); // unloaderPosition
    totalAlloc += stackSize * sizeof(Position); // paths
    totalAlloc += agentCount * sizeof(int); // pathSizes
    totalAlloc += agentCount * sizeof(int); // contraitsSizes
    totalAlloc += stackSize * sizeof(int); // gCost
    totalAlloc += stackSize * sizeof(int); // fCost
    totalAlloc += stackSize * sizeof(Position); // cameFrom
    totalAlloc += stackSize * sizeof(bool); // visited
    totalAlloc += stackSize * sizeof(Position); // openList
    totalAlloc += agentCount * (agentCount - 1) * sizeof(Constrait); // constrait
    totalAlloc += agentCount * sizeof(int); // numberConstrait
    totalAlloc += agentCount * sizeof(Color);
    size_t totalGlobalMem = getFreeRam();
    if (totalAlloc >= ((totalGlobalMem * 3) / 4)) {
        return false;
    }
    return true;
}

// ===================================== draw ====================

void Map::drawGrid(int cellWidth, int cellHeight) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Rectangle rect = Rectangle{ static_cast<float>(x * cellWidth), static_cast<float>(y * cellHeight), static_cast<float>(cellWidth), static_cast<float>(cellHeight) };
            if (m.grid[getTrueIndexGrid(width, x, y)] == OBSTACLE_SYMBOL) {
                DrawRectangleRec(rect, BLACK);
            }
            else if (m.grid[getTrueIndexGrid(width, x, y)] == LOADER_SYMBOL) {
                DrawRectangleRec(rect, LIGHTGRAY);
                DrawText("I", static_cast<float>(cellWidth) / 4 + rect.x, static_cast<float>(cellHeight) / 4 + rect.y, 20, DARKGRAY);
            }
            else if (m.grid[getTrueIndexGrid(width, x, y)] == UNLOADER_SYMBOL) {
                DrawRectangleRec(rect, LIGHTGRAY);
                DrawText("O", rect.x + static_cast<float>(cellWidth) / 4, rect.y + static_cast<float>(cellHeight) / 4, 20, DARKGRAY);
            }
            else {
                DrawRectangleLinesEx(rect, 1, RED);
            }
        }
    }
}

void Map::drawAgentPath(const int agentIndex, int cellWidth, int cellHeight) {
    if (m.pathSizesAgent[agentIndex] <= 0) {
        return; 
    }
    Color& c = colorAgents[agentIndex];
    Color pathColor = { c.r, c.g, c.b, 60 };
    Position *path = &m.pathsAgent[getTrueIndexPath(agentIndex, 0)];
    for (int i = 0; i < m.pathSizesAgent[agentIndex]; i++) {
        Position& pos = path[i];
        Rectangle rect = {
            static_cast<float>(pos.x * cellWidth),
            static_cast<float>(pos.y * cellHeight),
            static_cast<float>(cellWidth),
            static_cast<float>(cellHeight)
        };
        DrawRectangleRec(rect, pathColor);
    }
}

void Map::drawAgents(int cellWidth, int cellHeight) {
    for (int i = 0; i < agentCount; i++) {
        Agent& agent = m.agents[i];
        Color& c = colorAgents[i];
        int agentX = agent.x * cellWidth + cellWidth / 2;
        int agentY = agent.y * cellHeight + cellHeight / 2;
        DrawCircle(agentX, agentY, static_cast<float>(cellWidth) / 4, GREEN);
        DrawCircle(agentX, agentY, static_cast<float>(cellWidth) / 8, { c.r, c.g, c.b, c.a });
    }
    for (int i = 0; i < agentCount; i++) {
        drawAgentPath(i, cellWidth, cellHeight);
    }
}

void Map::draw(int screenWidth, int screenHeight) {
    int cellWidth = screenWidth / width;
    int cellHeight = screenHeight / height;
    drawGrid(cellWidth, cellHeight);
    drawAgents(cellWidth, cellHeight);
    drawSelect(cellWidth, cellHeight);
}

void Map::drawSelect(int cellWidth, int cellHeight){
    if (selected.x == -1) {
        return;
    }
    Color selectedColor = { 255, 0, 0, 191 }; // Èervená s 75% neprieh¾adnosou (191 = 75% z 255)
    Rectangle selectedRect = {
        static_cast<float>(selected.x * cellWidth),
        static_cast<float>(selected.y * cellHeight),
        static_cast<float>(cellWidth),
        static_cast<float>(cellHeight)
    };

    DrawRectangleLinesEx(selectedRect, 3, selectedColor);
}


