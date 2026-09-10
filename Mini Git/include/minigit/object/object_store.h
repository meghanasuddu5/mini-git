#pragma once

#include "minigit/object/hash.h"
#include "minigit/object/object.h"

#include <filesystem>
#include <optional>
#include <string>

namespace fs = std::filesystem;

namespace minigit {

/// The ObjectStore manages the `.minigit/objects/` directory.
///
/// It provides the core content-addressable storage operations:
///   - write: serialize + hash + store on disk
///   - read:  load from disk + validate + deserialize
///   - exists: check if an object ID is stored
///
/// Storage layout:
///   .minigit/objects/
///   ├── a9/
///   │   └── 48904f2f0f479b8f8...   ← first 2 hex chars = directory
///   ├── 3b/
///   │   └── 18e512d5b0c9abf...
///   └── ...
///
/// The fan-out directory structure (first 2 hex chars) prevents
/// any single directory from having too many files, which would
/// slow down filesystem operations on many OS/filesystem combinations.
class ObjectStore {
public:
    /// Construct an ObjectStore rooted at the given objects/ directory.
    explicit ObjectStore(const fs::path& objectsDir);

    // ── Write operations ────────────────────────────────────────────

    /// Store a Blob and return its object ID (hex hash).
    /// If the object already exists, this is a no-op (idempotent).
    std::string writeBlob(const Blob& blob);

    /// Store raw serialized data and return its object ID.
    /// This is the low-level method used by writeBlob (and later
    /// by writeTree and writeCommit).
    std::string writeRaw(const std::string& serializedData);

    // ── Read operations ─────────────────────────────────────────────

    /// Read raw serialized data by object ID.
    /// Returns std::nullopt if the object doesn't exist.
    /// Throws MiniGitException if the object is corrupted.
    std::optional<std::string> readRaw(const std::string& objectId) const;

    /// Read and deserialize a Blob by object ID.
    /// Throws MiniGitException if not found or corrupted.
    Blob readBlob(const std::string& objectId) const;

    // ── Query operations ────────────────────────────────────────────

    /// Check if an object with the given ID exists in the store.
    bool exists(const std::string& objectId) const;

private:
    /// Convert an object ID to its filesystem path.
    /// Example: "a948904f..." → objects/a9/48904f...
    fs::path objectPath(const std::string& objectId) const;

    /// Root of the objects directory (.minigit/objects/).
    fs::path objectsDir_;
};

} // namespace minigit
