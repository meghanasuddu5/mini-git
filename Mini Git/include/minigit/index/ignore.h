#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <regex>

namespace fs = std::filesystem;

namespace minigit {

/// Handles parsing and matching of .minigitignore files.
///
/// For this educational project, we support a simplified version
/// of gitignore patterns:
///   - Blank lines and lines starting with '#' are ignored.
///   - 'build/' ignores the build directory and all its contents.
///   - '*.exe' ignores all files ending in .exe.
///   - 'secret.txt' ignores files exactly named secret.txt.
class Ignore {
public:
    /// Construct from a .minigitignore file path.
    /// If the file doesn't exist, no patterns are loaded.
    explicit Ignore(const fs::path& ignoreFilePath);

    /// Check if a repository-relative path should be ignored.
    /// @param relPath Path with forward slashes (e.g., "src/main.cpp")
    [[nodiscard]] bool isIgnored(const std::string& relPath) const;

private:
    /// Convert a simple glob pattern to a regex string.
    std::string globToRegex(const std::string& pattern) const;

    /// The compiled regex patterns.
    std::vector<std::regex> patterns_;
};

} // namespace minigit
