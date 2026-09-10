#pragma once

#include "minigit/object/object.h"

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace minigit {

/// A Commit represents a snapshot of the repository at a specific point in time.
///
/// It links a Tree (the directory structure) to its history (parent commit(s))
/// and includes metadata like the author, timestamp, and a commit message.
///
/// Serialization format:
///   "commit <size>\0"
///   "tree <tree_hash>\n"
///   "parent <parent_hash>\n"     <-- 0, 1, or multiple for merge commits
///   "author <name> <timestamp>\n"
///   "\n"
///   "<commit message>\n"
class Commit {
public:
    /// Construct a Commit with multiple parents.
    Commit(std::string treeHash,
           std::vector<std::string> parentHashes,
           std::string author,
           uint64_t timestamp,
           std::string message);

    /// Convenience constructor with multiple parents using current system time.
    Commit(std::string treeHash,
           std::vector<std::string> parentHashes,
           std::string author,
           std::string message);

    /// Convenience constructor for 0 or 1 parent.
    Commit(std::string treeHash,
           std::optional<std::string> parentHash,
           std::string author,
           uint64_t timestamp,
           std::string message);

    /// Convenience constructor for 0 or 1 parent using current system time.
    Commit(std::string treeHash,
           std::optional<std::string> parentHash,
           std::string author,
           std::string message);

    // ── Getters ─────────────────────────────────────────────────────────────

    [[nodiscard]] const std::string& treeHash() const { return treeHash_; }
    [[nodiscard]] const std::vector<std::string>& parentHashes() const { return parentHashes_; }
    [[nodiscard]] std::optional<std::string> parentHash() const {
        return parentHashes_.empty() ? std::nullopt : std::make_optional(parentHashes_.front());
    }
    [[nodiscard]] const std::string& author() const { return author_; }
    [[nodiscard]] uint64_t timestamp() const { return timestamp_; }
    [[nodiscard]] const std::string& message() const { return message_; }

    // ── Serialization ───────────────────────────────────────────────────────

    /// Serialize this Commit into the storage format.
    [[nodiscard]] std::string serialize() const;

    /// Deserialize a Commit from storage format.
    /// Throws MiniGitException if the data is malformed.
    static Commit deserialize(const std::string& data);

private:
    std::string treeHash_;
    std::vector<std::string> parentHashes_;
    std::string author_;
    uint64_t timestamp_;
    std::string message_;
};

} // namespace minigit
