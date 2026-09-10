#include "minigit/repository/graph_analyzer.h"
#include "minigit/object/commit.h"
#include "minigit/object/object_store.h"
#include "minigit/utils/error.h"

#include <queue>
#include <unordered_set>
#include <vector>

namespace minigit {

std::optional<std::string> GraphAnalyzer::findMergeBase(
    const std::string& commitA,
    const std::string& commitB,
    const ObjectStore& store) {
    
    if (commitA.empty() || commitB.empty()) {
        return std::nullopt;
    }

    if (commitA == commitB) {
        return commitA;
    }

    // Step 1: Collect all ancestors of commitA (including commitA itself)
    std::unordered_set<std::string> ancestorsA;
    std::queue<std::string> queueA;

    queueA.push(commitA);
    ancestorsA.insert(commitA);

    while (!queueA.empty()) {
        std::string current = queueA.front();
        queueA.pop();

        auto rawOpt = store.readRaw(current);
        if (!rawOpt.has_value()) {
            continue;
        }

        try {
            Commit commit = Commit::deserialize(rawOpt.value());
            for (const auto& parent : commit.parentHashes()) {
                if (ancestorsA.find(parent) == ancestorsA.end()) {
                    ancestorsA.insert(parent);
                    queueA.push(parent);
                }
            }
        } catch (...) {
            continue;
        }
    }

    // Step 2: BFS backwards from commitB. The first commit found in ancestorsA is the LCA.
    std::unordered_set<std::string> visitedB;
    std::queue<std::string> queueB;

    queueB.push(commitB);
    visitedB.insert(commitB);

    while (!queueB.empty()) {
        std::string current = queueB.front();
        queueB.pop();

        if (ancestorsA.find(current) != ancestorsA.end()) {
            return current;
        }

        auto rawOpt = store.readRaw(current);
        if (!rawOpt.has_value()) {
            continue;
        }

        try {
            Commit commit = Commit::deserialize(rawOpt.value());
            for (const auto& parent : commit.parentHashes()) {
                if (visitedB.find(parent) == visitedB.end()) {
                    visitedB.insert(parent);
                    queueB.push(parent);
                }
            }
        } catch (...) {
            continue;
        }
    }

    return std::nullopt;
}

bool GraphAnalyzer::isAncestor(
    const std::string& ancestor,
    const std::string& descendant,
    const ObjectStore& store) {

    if (ancestor == descendant) {
        return true;
    }

    std::unordered_set<std::string> visited;
    std::queue<std::string> queue;

    queue.push(descendant);
    visited.insert(descendant);

    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();

        if (current == ancestor) {
            return true;
        }

        auto rawOpt = store.readRaw(current);
        if (!rawOpt.has_value()) {
            continue;
        }

        try {
            Commit commit = Commit::deserialize(rawOpt.value());
            for (const auto& parent : commit.parentHashes()) {
                if (visited.find(parent) == visited.end()) {
                    visited.insert(parent);
                    queue.push(parent);
                }
            }
        } catch (...) {
            continue;
        }
    }

    return false;
}

std::vector<std::string> GraphAnalyzer::getHistory(
    const std::string& startCommit,
    const ObjectStore& store) {

    std::vector<std::string> history;
    if (startCommit.empty()) {
        return history;
    }

    std::unordered_set<std::string> visited;
    std::queue<std::string> queue;

    queue.push(startCommit);
    visited.insert(startCommit);

    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();

        history.push_back(current);

        auto rawOpt = store.readRaw(current);
        if (!rawOpt.has_value()) {
            continue;
        }

        try {
            Commit commit = Commit::deserialize(rawOpt.value());
            for (const auto& parent : commit.parentHashes()) {
                if (visited.find(parent) == visited.end()) {
                    visited.insert(parent);
                    queue.push(parent);
                }
            }
        } catch (...) {
            continue;
        }
    }

    return history;
}

} // namespace minigit
