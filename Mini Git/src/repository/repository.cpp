#include "minigit/repository/repository.h"
#include "minigit/utils/error.h"
#include "minigit/utils/filesystem.h"

namespace minigit {

Repository::Repository(const fs::path& rootPath)
    : rootPath_(fs::canonical(rootPath)) {}

Repository Repository::init(const fs::path& path) {
    fs::path repoDir = path / REPO_DIR;

    // Check if already initialized
    if (utils::directoryExists(repoDir)) {
        throw RepositoryException(
            "repository already exists at '" + path.string() + "'");
    }

    // Create the directory structure:
    //   .minigit/
    //   .minigit/objects/
    //   .minigit/refs/
    //   .minigit/refs/heads/
    try {
        utils::createDirectoryIfNotExists(repoDir);
        utils::createDirectoryIfNotExists(repoDir / "objects");
        utils::createDirectoryIfNotExists(repoDir / "refs");
        utils::createDirectoryIfNotExists(repoDir / "refs" / "heads");

        // HEAD points to the default branch.
        // Format: "ref: refs/heads/main\n"
        // This means "I am currently on branch 'main'."
        utils::writeStringToFile(
            repoDir / "HEAD",
            std::string("ref: refs/heads/") + DEFAULT_BRANCH + "\n");

        // Empty index file — no files staged yet.
        utils::writeStringToFile(repoDir / "index", "");

        // Config file with a minimal default.
        // We store the author name here so commits can use it.
        utils::writeStringToFile(
            repoDir / "config",
            "[user]\n"
            "    name = MiniGit User\n");

    } catch (const FilesystemException& e) {
        throw RepositoryException(
            std::string("failed to initialize repository: ") + e.what());
    }

    return Repository(path);
}

Repository Repository::open(const fs::path& path) {
    // Search upward from the given path for a .minigit/ directory.
    auto root = utils::findRepoRoot(path);

    if (!root.has_value()) {
        throw RepositoryException(
            "not a minigit repository (or any parent up to filesystem root)");
    }

    // Validate the structure — at minimum, HEAD must exist
    fs::path headFile = root.value() / REPO_DIR / "HEAD";
    if (!utils::fileExists(headFile)) {
        throw RepositoryException(
            "corrupted repository: HEAD file is missing");
    }

    return Repository(root.value());
}

} // namespace minigit
