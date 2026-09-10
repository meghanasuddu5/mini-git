#pragma once

#include "minigit/object/object.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace minigit {

// Forward declaration
class ObjectStore;

/// A single entry in a Tree object.
///
/// Each entry represents either a file (blob) or subdirectory (tree)
/// within the directory that this Tree represents.
///
/// Fields:
///   - mode: "100644" for regular file, "040000" for directory
///   - type: ObjectType::Blob or ObjectType::Tree
///   - hash: SHA-256 object ID
///   - name: filename only (not a full path)
struct TreeEntry {
    std::string mode;
    ObjectType type;
    std::string hash;
    std::string name;

    bool operator==(const TreeEntry& other) const {
        return mode == other.mode && type == other.type
            && hash == other.hash && name == other.name;
    }
};

/// A Tree represents a directory snapshot.
///
/// Serialization format (stored in the object store):
///   "tree <size>\0"
///   "<mode> <type> <hash> <name>\n"
///   "<mode> <type> <hash> <name>\n"
///   ...
///
/// Entries are always sorted by name for deterministic hashing.
/// This means the same directory contents always produce the same
/// tree hash, regardless of the order files were added.
class Tree {
public:
    /// Construct a Tree from a list of entries.
    /// Entries will be sorted by name.
    explicit Tree(std::vector<TreeEntry> entries);

    /// Get the entries (sorted by name).
    [[nodiscard]] const std::vector<TreeEntry>& entries() const { return entries_; }

    /// Serialize this Tree into the storage format.
    [[nodiscard]] std::string serialize() const;

    /// Deserialize a Tree from storage format.
    /// Throws MiniGitException if the data is malformed.
    static Tree deserialize(const std::string& data);

    /// Find an entry by name. Returns nullptr if not found.
    [[nodiscard]] const TreeEntry* findEntry(const std::string& name) const;

private:
    std::vector<TreeEntry> entries_;
};

/// The TreeBuilder converts a flat list of (path, blobHash) pairs
/// (like the index entries) into a hierarchy of Tree objects.
///
/// Algorithm (bottom-up construction):
///   1. Group all paths by their directory components.
///   2. Start from the deepest directories.
///   3. Create a Tree for each directory containing its files (blobs)
///      and subdirectories (trees).
///   4. Store each Tree in the ObjectStore.
///   5. Return the root Tree's hash.
///
/// Example:
///   Input (from index):
///     "README.md"         → blob_hash_1
///     "src/main.cpp"      → blob_hash_2
///     "src/utils/help.h"  → blob_hash_3
///
///   Output tree hierarchy:
///     root tree:
///       100644 blob blob_hash_1 README.md
///       040000 tree tree_hash_A src
///     tree_hash_A (src/):
///       100644 blob blob_hash_2 main.cpp
///       040000 tree tree_hash_B utils
///     tree_hash_B (src/utils/):
///       100644 blob blob_hash_3 help.h
class TreeBuilder {
public:
    /// Add a file entry (path with forward slashes, blob hash).
    void addEntry(const std::string& path, const std::string& blobHash);

    /// Build the tree hierarchy, store all trees in the ObjectStore,
    /// and return the root tree's hash.
    ///
    /// Throws MiniGitException if no entries have been added.
    std::string build(ObjectStore& store);

private:
    /// Internal node representing a directory during tree construction.
    struct DirNode {
        /// Files directly in this directory: name → blob hash
        std::map<std::string, std::string> files;

        /// Subdirectories: name → child DirNode
        std::map<std::string, DirNode> subdirs;
    };

    /// Recursively build trees from a DirNode, bottom-up.
    /// Returns the tree hash for this directory.
    std::string buildNode(DirNode& node, ObjectStore& store);

    /// The root directory node.
    DirNode root_;
};

} // namespace minigit
