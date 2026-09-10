#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace minigit {

class Index;
class ObjectStore;

/// Handles checking out branches and commits into the working tree.
class Checkout {
public:
    /// Construct a Checkout utility.
    /// @param repoRoot    The root directory of the repository.
    /// @param objectsDir  The `.minigit/objects/` directory.
    Checkout(const fs::path& repoRoot, const fs::path& objectsDir);

    /// Checkout a specific commit hash.
    /// This will:
    /// 1. Remove all files currently tracked by the given index.
    /// 2. Traverse the commit's tree and write all its files to the working directory.
    /// 3. Update the index to exactly match the target commit's tree.
    ///
    /// Note: This is an educational implementation that assumes a clean working
    /// tree. It will overwrite uncommitted changes to tracked files.
    ///
    /// @param commitHash The SHA-256 hash of the commit to checkout.
    /// @param index      The index to read from (for cleanup) and update.
    void checkoutCommit(const std::string& commitHash, Index& index);

private:
    /// Recursively traverse a tree and write files to disk, collecting them for the index.
    void checkoutTreeRecursive(const std::string& treeHash,
                               const std::string& currentPath,
                               Index& index);

    fs::path repoRoot_;
    fs::path objectsDir_;
};

} // namespace minigit
