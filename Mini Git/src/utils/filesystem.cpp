#include "minigit/utils/filesystem.h"
#include "minigit/utils/error.h"

#include <fstream>
#include <sstream>

namespace minigit::utils {

void createDirectoryIfNotExists(const fs::path& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    if (ec) {
        throw FilesystemException(
            "cannot create directory '" + path.string() + "': " + ec.message());
    }
}

bool fileExists(const fs::path& path) {
    return fs::exists(path) && fs::is_regular_file(path);
}

bool directoryExists(const fs::path& path) {
    return fs::exists(path) && fs::is_directory(path);
}

std::string readFileToString(const fs::path& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw FilesystemException(
            "cannot open file for reading: '" + path.string() + "'");
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    if (file.bad()) {
        throw FilesystemException(
            "error reading file: '" + path.string() + "'");
    }

    return ss.str();
}

void writeStringToFile(const fs::path& path, const std::string& content) {
    // Ensure the parent directory exists
    if (path.has_parent_path()) {
        createDirectoryIfNotExists(path.parent_path());
    }

    std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw FilesystemException(
            "cannot open file for writing: '" + path.string() + "'");
    }

    file << content;

    if (file.bad()) {
        throw FilesystemException(
            "error writing file: '" + path.string() + "'");
    }
}

std::optional<fs::path> findRepoRoot(const fs::path& startPath) {
    // Canonicalize so we don't loop on symlinks or relative paths
    fs::path current = fs::canonical(startPath);

    while (true) {
        fs::path candidate = current / ".minigit";
        if (directoryExists(candidate)) {
            return current;
        }

        fs::path parent = current.parent_path();

        // If parent == current, we've reached the root of the filesystem
        if (parent == current) {
            return std::nullopt;
        }

        current = parent;
    }
}

} // namespace minigit::utils
