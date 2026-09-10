#pragma once

#include "minigit/index/ignore.h"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace minigit {

/// A single entry in the staging index.
///
/// Records that a specific file path maps to a specific blob hash.
/// The path is always stored as a forward-slash relative path
/// (e.g., "src/main.cpp") regardless of the OS, for portability.
struct IndexEntry {
    std::string path;       // Relative path from repo root (forward slashes)
    std::string blobHash;   // SHA-256 hash of the blob object

    bool operator==(const IndexEntry& other) const {
        return path == other.path && blobHash == other.blobHash;
    }
};

/// Status of a file in the working tree relative to the index.
enum class FileStatus {
    Untracked,  // File exists in working tree but not in index
    Modified,   // File exists in both but content differs
    Staged,     // File is in the index (ready to commit)
    Deleted     // File is in the index but not in working tree
};

/// Result of a status check for a single file.
struct StatusEntry {
    std::string path;
    FileStatus status;
};

/// The Index manages the staging area between the working tree and commits.
///
/// It is persisted as the `.minigit/index` file.
///
/// Conceptual flow:
///   1. User edits files in working tree
///   2. `minigit add` stages files → updates index + writes blobs
///   3. `minigit commit` reads index → builds tree → creates commit
///   4. `minigit status` compares working tree vs index
///
/// The index stores entries sorted by path. This ensures deterministic
/// serialization, which is important for reproducibility and testing.
class Index {
public:
    /// Construct an index associated with a repository.
    /// @param indexPath   Path to the .minigit/index file
    /// @param objectsPath Path to the .minigit/objects/ directory
    /// @param repoRoot    Path to the repository root (for resolving relative paths)
    Index(const fs::path& indexPath,
          const fs::path& objectsPath,
          const fs::path& repoRoot);

    // ── Staging operations ──────────────────────────────────────────

    /// Stage a single file. Reads its contents, creates a blob,
    /// stores it in the object store, and updates the index.
    /// @param filePath Absolute or relative path to the file
    void addFile(const fs::path& filePath);

    /// Stage all files in the repository (recursive).
    /// Equivalent to `minigit add .`
    void addAll();

    /// Set an index entry directly (used by checkout/merge).
    void setEntry(const std::string& relativePath, const std::string& blobHash);

    /// Remove a file from the index (unstage it).
    /// Does NOT delete the file from the working tree.
    void removeFile(const std::string& relativePath);

    // ── Query operations ────────────────────────────────────────────

    /// Get all index entries, sorted by path.
    [[nodiscard]] const std::map<std::string, IndexEntry>& entries() const {
        return entries_;
    }

    /// Check if a file path is in the index.
    [[nodiscard]] bool contains(const std::string& relativePath) const;

    /// Get the blob hash for a staged file, or nullopt if not staged.
    [[nodiscard]] std::optional<std::string> getBlobHash(
        const std::string& relativePath) const;

    /// Compute the status of the working tree relative to the index.
    /// Returns a sorted list of StatusEntry objects.
    [[nodiscard]] std::vector<StatusEntry> computeStatus() const;

    // ── Persistence ─────────────────────────────────────────────────

    /// Load the index from disk. Called in constructor.
    void load();

    /// Save the index to disk.
    void save() const;

    // ── Utilities ───────────────────────────────────────────────────

    /// Convert an absolute path to a repo-relative path with forward slashes.
    [[nodiscard]] std::string toRelativePath(const fs::path& absolutePath) const;

    /// Get the number of entries in the index.
    [[nodiscard]] size_t size() const { return entries_.size(); }

    /// Clear all entries (used in testing).
    void clear();

private:
    /// Collect all files in the working tree (excluding .minigit/).
    /// Returns repo-relative paths with forward slashes.
    [[nodiscard]] std::vector<std::string> collectWorkingTreeFiles() const;

    /// Compute the blob hash for a working tree file (without storing it).
    [[nodiscard]] std::string computeFileHash(const fs::path& absolutePath) const;

    /// Path to the .minigit/index file.
    fs::path indexPath_;

    /// Path to the .minigit/objects/ directory.
    fs::path objectsPath_;

    /// Root directory of the repository.
    fs::path repoRoot_;

    /// The staged entries, keyed by relative path.
    /// Using std::map keeps entries sorted by path automatically.
    std::map<std::string, IndexEntry> entries_;

    /// The ignore rules loaded from .minigitignore.
    Ignore ignore_;
};

} // namespace minigit
