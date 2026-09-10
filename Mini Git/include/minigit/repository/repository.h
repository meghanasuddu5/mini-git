#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace minigit {

/// The Repository class encapsulates the `.minigit/` directory.
///
/// It provides two static factory methods:
///   - init()  — creates a new repository
///   - open()  — finds and validates an existing repository
///
/// Once constructed, it provides paths to all internal directories
/// and files (objects/, refs/, HEAD, index, config).
///
/// Design note: We use static factory methods instead of constructors
/// because initialization can fail, and factory methods can return
/// descriptive errors before constructing the object. This follows
/// the "Resource Acquisition Is Initialization" (RAII) principle —
/// if you have a Repository object, it is valid.
class Repository {
public:
    /// The name of the hidden directory inside a repository.
    static constexpr const char* REPO_DIR = ".minigit";

    /// The default branch name for new repositories.
    static constexpr const char* DEFAULT_BRANCH = "main";

    /// Initialize a new repository at the given path.
    /// Creates the .minigit/ directory structure.
    /// Throws RepositoryException if already initialized or on failure.
    static Repository init(const fs::path& path);

    /// Open an existing repository by searching upward from the given path.
    /// Throws RepositoryException if no repository is found.
    static Repository open(const fs::path& path);

    // ── Path accessors ────────────────────────────────────────────────

    /// Root directory of the working tree (parent of .minigit/).
    [[nodiscard]] const fs::path& rootPath() const { return rootPath_; }

    /// Path to .minigit/ itself.
    [[nodiscard]] fs::path minigitPath() const { return rootPath_ / REPO_DIR; }

    /// Path to .minigit/objects/
    [[nodiscard]] fs::path objectsPath() const { return minigitPath() / "objects"; }

    /// Path to .minigit/refs/
    [[nodiscard]] fs::path refsPath() const { return minigitPath() / "refs"; }

    /// Path to .minigit/refs/heads/
    [[nodiscard]] fs::path headsPath() const { return refsPath() / "heads"; }

    /// Path to .minigit/HEAD
    [[nodiscard]] fs::path headPath() const { return minigitPath() / "HEAD"; }

    /// Path to .minigit/index
    [[nodiscard]] fs::path indexPath() const { return minigitPath() / "index"; }

    /// Path to .minigit/config
    [[nodiscard]] fs::path configPath() const { return minigitPath() / "config"; }

private:
    /// Private constructor — use init() or open() instead.
    explicit Repository(const fs::path& rootPath);

    /// The root of the working tree.
    fs::path rootPath_;
};

} // namespace minigit
