#include <queue>
#include "fill_init_convert.h"
#include "a_star_algo.h"
#include "compute.h"
#include "solve_conflicts.h" 

std::vector<Conflict> detectConflicts(const CBSNode& node) {
    std::vector<Conflict> conflicts;
    int agentCount = node.paths.size();
    int maxTime = 0;
    for (const auto& path : node.paths) {
        maxTime = std::max(maxTime, (int)path.size());
    }
    std::unordered_set<std::pair<int, int>, PairHash, PairEqual> reportedPairs;
    
    for (int t = 0; t < maxTime; t++) {
        for (int i = 0; i < agentCount; i++) {
            if (t >= node.paths[i].size()) continue;
            Position posA = node.paths[i][t];

            for (int j = i + 1; j < agentCount; j++) {
                if (t >= node.paths[j].size()) continue;
                Position posB = node.paths[j][t];

                if (posA == posB) {
                    if (reportedPairs.emplace(std::minmax(i, j)).second) {
                        conflicts.push_back({ i, j, t });
                    }
                }
                if (t + 1 < node.paths[i].size() && t + 1 < node.paths[j].size()) {
                    Position nextA = node.paths[i][t + 1];
                    Position nextB = node.paths[j][t + 1];
                    if (posA == nextB && posB == nextA) {
                        if (reportedPairs.emplace(std::minmax(i, j)).second) {
                            conflicts.push_back({ i, j, t });
                        }
                    }
                }
            }
        }
    }
    return conflicts;
}



bool isIgnorableConflict(const Conflict& conflict, const CBSNode& node) {
    auto pathA = node.paths[conflict.agentA];
    auto pathB = node.paths[conflict.agentB];
    if (conflict.timeStep == 0) return false;
    if (pathA.size() <= conflict.timeStep || pathB.size() <= conflict.timeStep) return false;
    bool agentAStanding = pathA[conflict.timeStep] == pathA[conflict.timeStep - 1];
    bool agentBStanding = pathB[conflict.timeStep] == pathB[conflict.timeStep - 1];
	if (pathA.size() > conflict.timeStep + 1) {
		agentAStanding |= (pathA[conflict.timeStep] == pathA[conflict.timeStep + 1]);
	}
	if (pathB.size() > conflict.timeStep + 1) {
		agentBStanding |= (pathB[conflict.timeStep] == pathB[conflict.timeStep + 1]);
	}
    return agentAStanding || agentBStanding;
}

std::vector<CBSNode> expandNode(const CBSNode& node, const Conflict& conflict) {
    std::vector<CBSNode> result;
    for (int agentToSlow : {conflict.agentA, conflict.agentB}) {
        CBSNode child = node;
        if (child.paths[agentToSlow].size() > conflict.timeStep) {
            Position duplicatePos;
            duplicatePos = child.paths[agentToSlow][conflict.timeStep];
            child.paths[agentToSlow].insert(child.paths[agentToSlow].begin() + conflict.timeStep, duplicatePos);
        }

        result.push_back(child);
    }

    return result;
}

CBSNode solveCBS(const std::vector<std::vector<Position>>& initialPaths) {
    std::priority_queue<CBSNode> openList;
    CBSNode root;
    root.paths = initialPaths;
    root.turn = 0;

    auto conflicts = detectConflicts(root);
    conflicts.erase(
        std::remove_if(conflicts.begin(), conflicts.end(),
            [&](const Conflict& c) { return isIgnorableConflict(c, root); }),
        conflicts.end()
    );
	root.detected_conflicts = conflicts;
	root.conflictCount = conflicts.size();
    openList.push(root);
    while (!openList.empty()) {
        CBSNode current = openList.top();
        openList.pop();
        if (current.conflictCount == 0) {
            return current;
        }
        auto expansions = expandNode(current, current.detected_conflicts[0]);
        for (auto& child : expansions) {
            child.turn = current.turn+1;
            conflicts = detectConflicts(child);
			conflicts.erase(
				std::remove_if(conflicts.begin(), conflicts.end(),
					[&](const Conflict& c) { return isIgnorableConflict(c, child); }),
				conflicts.end()
			);
            child.detected_conflicts = conflicts;
			child.conflictCount = conflicts.size();
            openList.push(child);
        }
    }
    return {};
}
