#include "minigit/repository/checkout.h"
#include "minigit/index/index.h"
#include "minigit/object/commit.h"
#include "minigit/object/object_store.h"
#include "minigit/object/tree.h"
#include "minigit/utils/error.h"
#include "minigit/utils/filesystem.h"

namespace minigit {

Checkout::Checkout(const fs::path& repoRoot, const fs::path& objectsDir)
    : repoRoot_(fs::canonical(repoRoot))
    , objectsDir_(objectsDir) {}

void Checkout::checkoutCommit(const std::string& commitHash, Index& index) {
    ObjectStore store(objectsDir_);

    // 1. Read the target commit
    auto rawCommitOpt = store.readRaw(commitHash);
    if (!rawCommitOpt.has_value()) {
        throw MiniGitException("checkout failed: commit " + commitHash + " not found");
    }
    Commit commit = Commit::deserialize(rawCommitOpt.value());

    // 2. Clean up working directory (remove currently tracked files)
    //    We iterate through the index and delete the files it tracks.
    //    We do NOT delete untracked/ignored files.
    for (const auto& [relPath, entry] : index.entries()) {
        fs::path absPath = repoRoot_ / relPath;
        if (utils::fileExists(absPath)) {
            fs::remove(absPath);
        }
    }

    // 3. Clear the index since we are completely rebuilding it from the target tree
    index.clear();

    // 4. Traverse the target tree and checkout files
    checkoutTreeRecursive(commit.treeHash(), "", index);

    // 5. Save the updated index
    index.save();
}

void Checkout::checkoutTreeRecursive(const std::string& treeHash,
                                     const std::string& currentPath,
                                     Index& index) {
    ObjectStore store(objectsDir_);

    auto rawTreeOpt = store.readRaw(treeHash);
    if (!rawTreeOpt.has_value()) {
        throw MiniGitException("checkout failed: missing tree " + treeHash);
    }
    Tree tree = Tree::deserialize(rawTreeOpt.value());

    for (const auto& entry : tree.entries()) {
        std::string relPath = currentPath.empty() ? entry.name : currentPath + "/" + entry.name;
        fs::path absPath = repoRoot_ / relPath;

        if (entry.type == ObjectType::Blob) {
            // Read blob from object store
            auto rawBlobOpt = store.readRaw(entry.hash);
            if (!rawBlobOpt.has_value()) {
                throw MiniGitException("checkout failed: missing blob " + entry.hash);
            }
            Blob blob = Blob::deserialize(rawBlobOpt.value());

            // Ensure parent directories exist
            fs::create_directories(absPath.parent_path());

            // Write content to disk
            utils::writeStringToFile(absPath, blob.content());

            // Add directly to index with known blob hash
            index.setEntry(relPath, entry.hash);

        } else if (entry.type == ObjectType::Tree) {
            // Recurse into subdirectory
            checkoutTreeRecursive(entry.hash, relPath, index);
        }
    }
}

} // namespace minigit
