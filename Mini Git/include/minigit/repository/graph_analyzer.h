#pragma once

#include <optional>
#include <string>
#include <vector>

namespace minigit {

class ObjectStore;

/// Analyzes the commit Directed Acyclic Graph (DAG).
///
/// Provides graph traversal algorithms:
///   - Lowest Common Ancestor (LCA) / Merge Base detection
///   - Ancestry / Reachability checks (fast-forward determination)
class GraphAnalyzer {
public:
    /// Find the lowest common ancestor (Merge Base) of two commits.
    ///
    /// Algorithm:
    ///   1. BFS backwards from commitA to collect all reachable ancestor hashes.
    ///   2. BFS backwards from commitB; the first ancestor encountered that is in
    ///      commitA's ancestor set is the closest common ancestor (LCA).
    ///
    /// Returns std::nullopt if the commits share no common ancestor (disjoint graphs).
    static std::optional<std::string> findMergeBase(
        const std::string& commitA,
        const std::string& commitB,
        const ObjectStore& store);

    /// Check if `ancestor` is an ancestor of `descendant` (reachable via parent pointers).
    /// Returns true if ancestor == descendant.
    static bool isAncestor(
        const std::string& ancestor,
        const std::string& descendant,
        const ObjectStore& store);

    /// Collect all ancestor commit hashes in topological/reverse-chronological order.
    static std::vector<std::string> getHistory(
        const std::string& startCommit,
        const ObjectStore& store);
};

} // namespace minigit
