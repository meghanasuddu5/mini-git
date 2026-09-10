#include "minigit/object/object_store.h"
#include "minigit/utils/error.h"
#include "minigit/utils/filesystem.h"

namespace minigit {

ObjectStore::ObjectStore(const fs::path& objectsDir)
    : objectsDir_(objectsDir) {}

// ─── Write ──────────────────────────────────────────────────────────────────

std::string ObjectStore::writeBlob(const Blob& blob) {
    return writeRaw(blob.serialize());
}

std::string ObjectStore::writeRaw(const std::string& serializedData) {
    // Step 1: Compute the SHA-256 hash of the serialized data.
    //         This hash IS the object ID.
    std::string objectId = SHA256::hashString(serializedData);

    // Step 2: Check if this object already exists (deduplication).
    //         If so, we're done — content-addressable storage is idempotent.
    if (exists(objectId)) {
        return objectId;
    }

    // Step 3: Write the object to disk.
    //         Path: objects/<first 2 hex chars>/<remaining hex chars>
    fs::path path = objectPath(objectId);
    utils::writeStringToFile(path, serializedData);

    return objectId;
}

// ─── Read ───────────────────────────────────────────────────────────────────

std::optional<std::string> ObjectStore::readRaw(const std::string& objectId) const {
    fs::path path = objectPath(objectId);

    if (!utils::fileExists(path)) {
        return std::nullopt;
    }

    std::string data = utils::readFileToString(path);

    // Integrity check: recompute the hash and compare.
    // This detects silent corruption (e.g., disk errors, manual tampering).
    std::string actualHash = SHA256::hashString(data);
    if (actualHash != objectId) {
        throw MiniGitException(
            "corrupted object: expected hash " + objectId
            + " but content hashes to " + actualHash);
    }

    return data;
}

Blob ObjectStore::readBlob(const std::string& objectId) const {
    auto raw = readRaw(objectId);
    if (!raw.has_value()) {
        throw MiniGitException("object not found: " + objectId);
    }
    return Blob::deserialize(raw.value());
}

// ─── Exists ─────────────────────────────────────────────────────────────────

bool ObjectStore::exists(const std::string& objectId) const {
    return utils::fileExists(objectPath(objectId));
}

// ─── Path Helpers ───────────────────────────────────────────────────────────

fs::path ObjectStore::objectPath(const std::string& objectId) const {
    // Fan-out: first 2 hex characters become a subdirectory.
    // Example: "a948904f2f..." → objects/a9/48904f2f...
    if (objectId.size() < 3) {
        throw MiniGitException("invalid object ID: too short");
    }
    return objectsDir_ / objectId.substr(0, 2) / objectId.substr(2);
}

} // namespace minigit
