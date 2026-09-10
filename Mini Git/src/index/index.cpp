#include "minigit/index/index.h"
#include "minigit/object/hash.h"
#include "minigit/object/object.h"
#include "minigit/object/object_store.h"
#include "minigit/utils/error.h"
#include "minigit/utils/filesystem.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace minigit {

Index::Index(const fs::path& indexPath,
             const fs::path& objectsPath,
             const fs::path& repoRoot)
    : indexPath_(indexPath)
    , objectsPath_(objectsPath)
    , repoRoot_(fs::canonical(repoRoot))
    , ignore_(repoRoot / ".minigitignore") {
    load();
}

// ─── Staging Operations ─────────────────────────────────────────────────────

void Index::addFile(const fs::path& filePath) {
    // Resolve to absolute path
    fs::path absPath = fs::canonical(filePath);

    if (!utils::fileExists(absPath)) {
        throw MiniGitException("pathspec '" + filePath.string() + "' did not match any files");
    }

    // Convert to repo-relative path
    std::string relPath = toRelativePath(absPath);

    // Read file contents and create a blob
    std::string content = utils::readFileToString(absPath);
    Blob blob(content);

    // Store the blob in the object store
    ObjectStore store(objectsPath_);
    std::string blobHash = store.writeBlob(blob);

    // Update the index entry
    entries_[relPath] = IndexEntry{relPath, blobHash};

    // Persist to disk
    save();
}

void Index::addAll() {
    // Collect all files in the working tree
    auto files = collectWorkingTreeFiles();

    ObjectStore store(objectsPath_);

    for (const auto& relPath : files) {
        fs::path absPath = repoRoot_ / relPath;
        std::string content = utils::readFileToString(absPath);
        Blob blob(content);
        std::string blobHash = store.writeBlob(blob);
        entries_[relPath] = IndexEntry{relPath, blobHash};
    }

    // Also remove entries for files that no longer exist
    std::vector<std::string> toRemove;
    for (const auto& [path, entry] : entries_) {
        fs::path absPath = repoRoot_ / path;
        if (!utils::fileExists(absPath)) {
            toRemove.push_back(path);
        }
    }
    for (const auto& path : toRemove) {
        entries_.erase(path);
    }

    save();
}

void Index::setEntry(const std::string& relativePath, const std::string& blobHash) {
    entries_[relativePath] = IndexEntry{relativePath, blobHash};
    save();
}

void Index::removeFile(const std::string& relativePath) {
    auto it = entries_.find(relativePath);
    if (it == entries_.end()) {
        throw MiniGitException("pathspec '" + relativePath + "' is not in the index");
    }
    entries_.erase(it);
    save();
}

// ─── Query Operations ──────────────────────────────────────────────────────

bool Index::contains(const std::string& relativePath) const {
    return entries_.find(relativePath) != entries_.end();
}

std::optional<std::string> Index::getBlobHash(
    const std::string& relativePath) const {
    auto it = entries_.find(relativePath);
    if (it == entries_.end()) {
        return std::nullopt;
    }
    return it->second.blobHash;
}

std::vector<StatusEntry> Index::computeStatus() const {
    std::vector<StatusEntry> result;

    // Step 1: Check all indexed files against the working tree
    for (const auto& [relPath, entry] : entries_) {
        fs::path absPath = repoRoot_ / relPath;

        if (!utils::fileExists(absPath)) {
            // File was deleted from working tree
            result.push_back({relPath, FileStatus::Deleted});
        } else {
            // File exists — check if it was modified
            std::string currentHash = computeFileHash(absPath);
            if (currentHash != entry.blobHash) {
                result.push_back({relPath, FileStatus::Modified});
            } else {
                result.push_back({relPath, FileStatus::Staged});
            }
        }
    }

    // Step 2: Find untracked files (in working tree but not in index)
    auto workingFiles = collectWorkingTreeFiles();
    for (const auto& relPath : workingFiles) {
        if (!contains(relPath)) {
            result.push_back({relPath, FileStatus::Untracked});
        }
    }

    // Sort by path for deterministic output
    std::sort(result.begin(), result.end(),
              [](const StatusEntry& a, const StatusEntry& b) {
                  return a.path < b.path;
              });

    return result;
}

// ─── Persistence ────────────────────────────────────────────────────────────

void Index::load() {
    entries_.clear();

    if (!utils::fileExists(indexPath_)) {
        return; // No index file yet — empty index
    }

    std::string content = utils::readFileToString(indexPath_);
    if (content.empty()) {
        return; // Empty index file
    }

    // Parse line by line: "<blob_hash> <path>\n"
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        // Find the first space — everything before is hash, after is path
        size_t spacePos = line.find(' ');
        if (spacePos == std::string::npos || spacePos == 0) {
            throw MiniGitException("malformed index entry: '" + line + "'");
        }

        std::string hash = line.substr(0, spacePos);
        std::string path = line.substr(spacePos + 1);

        if (hash.size() != SHA256::HEX_DIGEST_SIZE) {
            throw MiniGitException("malformed index entry: invalid hash length");
        }

        entries_[path] = IndexEntry{path, hash};
    }
}

void Index::save() const {
    // Serialize all entries: "<blob_hash> <path>\n"
    // Entries are already sorted because we use std::map
    std::ostringstream stream;
    for (const auto& [path, entry] : entries_) {
        stream << entry.blobHash << " " << entry.path << "\n";
    }
    utils::writeStringToFile(indexPath_, stream.str());
}

// ─── Utilities ──────────────────────────────────────────────────────────────

std::string Index::toRelativePath(const fs::path& absolutePath) const {
    // Make the path relative to the repo root
    fs::path relPath = fs::relative(absolutePath, repoRoot_);

    // Convert to forward slashes for portability
    std::string result = relPath.generic_string();
    return result;
}

void Index::clear() {
    entries_.clear();
    save();
}

std::vector<std::string> Index::collectWorkingTreeFiles() const {
    std::vector<std::string> files;

    for (const auto& entry : fs::recursive_directory_iterator(repoRoot_)) {
        if (!entry.is_regular_file()) continue;

        fs::path relPath = fs::relative(entry.path(), repoRoot_);
        std::string relStr = relPath.generic_string();

        if (ignore_.isIgnored(relStr)) {
            continue;
        }

        files.push_back(relStr);
    }

    std::sort(files.begin(), files.end());
    return files;
}

std::string Index::computeFileHash(const fs::path& absolutePath) const {
    // Read file, create a blob, and compute its hash.
    // We DON'T write to the object store here — this is just for comparison.
    std::string content = utils::readFileToString(absolutePath);
    Blob blob(content);
    return SHA256::hashString(blob.serialize());
}

} // namespace minigit
