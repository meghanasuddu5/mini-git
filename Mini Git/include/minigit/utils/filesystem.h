#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace fs = std::filesystem;

namespace minigit::utils {

/// Create a directory (and all parent directories) if it doesn't exist.
/// Throws FilesystemException on failure.
void createDirectoryIfNotExists(const fs::path& path);

/// Check if a file exists at the given path.
bool fileExists(const fs::path& path);

/// Check if a directory exists at the given path.
bool directoryExists(const fs::path& path);

/// Read an entire file into a string.
/// Throws FilesystemException if the file cannot be read.
std::string readFileToString(const fs::path& path);

/// Write a string to a file, creating it if it doesn't exist,
/// overwriting it if it does.
/// Throws FilesystemException on failure.
void writeStringToFile(const fs::path& path, const std::string& content);

/// Walk up the directory tree from `startPath` looking for a `.minigit/`
/// directory. Returns the path containing `.minigit/` if found,
/// or std::nullopt if we reach the filesystem root without finding one.
///
/// This is how commands like `minigit status` work from subdirectories —
/// they find the repo root automatically.
std::optional<fs::path> findRepoRoot(const fs::path& startPath);

} // namespace minigit::utils
