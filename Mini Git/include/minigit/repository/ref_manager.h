#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace minigit {

/// Manages Git references (branches and HEAD).
///
/// Branches are stored as files in `.minigit/refs/heads/`.
/// The contents of a branch file is the 64-character SHA-256 commit hash it points to.
///
/// HEAD is stored in `.minigit/HEAD`. It can either:
/// 1. Point to a branch: "ref: refs/heads/main"
/// 2. Be detached (point directly to a commit): "a948904f..."
class RefManager {
public:
    /// Construct from the root of the `.minigit` directory.
    explicit RefManager(const fs::path& minigitDir);

    // ── HEAD Operations ─────────────────────────────────────────────

    /// Resolve HEAD to a commit hash.
    /// Returns std::nullopt if HEAD points to a branch that doesn't exist yet
    /// (e.g., initial repository state).
    [[nodiscard]] std::optional<std::string> resolveHead() const;

    /// Read the exact contents of HEAD.
    [[nodiscard]] std::string readHead() const;

    /// Update HEAD to point to a specific branch.
    void updateHeadToBranch(const std::string& branchName);

    /// Update HEAD to point directly to a commit (detached HEAD).
    void updateHeadToCommit(const std::string& commitHash);

    // ── Branch Operations ───────────────────────────────────────────

    /// List all branches in `refs/heads/`.
    [[nodiscard]] std::vector<std::string> listBranches() const;

    /// Check if a branch exists.
    [[nodiscard]] bool branchExists(const std::string& branchName) const;

    /// Create a new branch pointing to the given commit hash.
    /// Throws if the branch already exists.
    void createBranch(const std::string& branchName, const std::string& commitHash);

    /// Resolve a branch name to a commit hash.
    /// Returns std::nullopt if the branch doesn't exist.
    [[nodiscard]] std::optional<std::string> resolveBranch(const std::string& branchName) const;

    /// Update an existing branch to point to a new commit hash.
    /// Throws if the branch does not exist.
    void updateBranch(const std::string& branchName, const std::string& commitHash);

    /// Delete a branch.
    /// Throws if the branch does not exist or if it is currently checked out.
    void deleteBranch(const std::string& branchName);

    /// Rename a branch.
    /// Throws if old branch does not exist or new branch already exists.
    void renameBranch(const std::string& oldName, const std::string& newName);

    /// Get the current branch name (if not detached HEAD).
    [[nodiscard]] std::optional<std::string> getCurrentBranch() const;

    /// Resolve a reference string (e.g. "HEAD", branch name, or commit hash) to a commit hash.
    [[nodiscard]] std::optional<std::string> resolveRef(const std::string& refOrHash) const;

private:
    fs::path minigitDir_;
    fs::path headPath_;
    fs::path refsHeadsDir_;
};

} // namespace minigit
