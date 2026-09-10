#pragma once

#include "minigit/object/object_store.h"

#include <map>
#include <string>
#include <vector>

namespace minigit {

/// Result of a 3-way tree merge operation.
struct MergeResult {
    bool hasConflicts = false;
    std::vector<std::string> conflictedFiles;
    std::vector<std::string> mergedFiles;
    std::vector<std::string> deletedFiles;

    /// Map from relative path to resolved/conflict-marked file content
    std::map<std::string, std::string> fileContents;

    /// Map from relative path to blob hash (only for cleanly resolved files)
    std::map<std::string, std::string> fileBlobs;
};

/// 3-way merge engine for MiniGit.
class MergeEngine {
public:
    /// Perform a 3-way merge of two trees using a base tree.
    ///
    /// @param baseTreeHash   The tree hash of the merge base (LCA commit). Can be empty for root merges.
    /// @param oursTreeHash   The tree hash of the current HEAD commit (OURS).
    /// @param theirsTreeHash The tree hash of the branch being merged in (THEIRS).
    /// @param store          The object store.
    /// @param ourBranchName  Name for conflict markers (e.g., "HEAD" or "main").
    /// @param theirBranchName Name for conflict markers (e.g., "feature").
    static MergeResult mergeTrees(
        const std::string& baseTreeHash,
        const std::string& oursTreeHash,
        const std::string& theirsTreeHash,
        ObjectStore& store,
        const std::string& ourBranchName = "HEAD",
        const std::string& theirBranchName = "THEIRS");

    /// Perform a line-by-line 3-way merge of two file contents against a base content.
    /// Returns pair<merged_content, has_conflict>.
    static std::pair<std::string, bool> merge3WayLines(
        const std::string& baseContent,
        const std::string& oursContent,
        const std::string& theirsContent,
        const std::string& ourBranchName,
        const std::string& theirBranchName);

    /// Flatten a tree hierarchy into a flat map of (relative path -> blob hash).
    static std::map<std::string, std::string> flattenTree(
        const std::string& treeHash,
        const ObjectStore& store);

private:
    static void flattenTreeRecursive(
        const std::string& treeHash,
        const std::string& currentPath,
        const ObjectStore& store,
        std::map<std::string, std::string>& outFiles);
};

} // namespace minigit
