#pragma once

#include <string>
#include <string_view>

namespace minigit {

/// Represents the type of an object stored in the object store.
/// These map directly to the header prefix in the serialized format.
enum class ObjectType {
    Blob,   // Raw file contents
    Tree,   // Directory listing (Phase 4)
    Commit  // Snapshot with metadata (Phase 5)
};

/// Convert ObjectType to its string representation for serialization.
inline std::string_view objectTypeToString(ObjectType type) {
    switch (type) {
        case ObjectType::Blob:   return "blob";
        case ObjectType::Tree:   return "tree";
        case ObjectType::Commit: return "commit";
    }
    return "unknown";
}

/// Parse a string into an ObjectType.
/// Returns Blob as default if unrecognized (caller should validate).
inline ObjectType stringToObjectType(std::string_view str) {
    if (str == "blob")   return ObjectType::Blob;
    if (str == "tree")   return ObjectType::Tree;
    if (str == "commit") return ObjectType::Commit;
    return ObjectType::Blob; // Should not reach here with valid data
}

/// A Blob represents the contents of a single file.
///
/// In MiniGit's object model, a Blob is the simplest object type.
/// It stores ONLY the raw file data — no filename, no permissions,
/// no metadata. Those are stored in the Tree that references this Blob.
///
/// Serialization format:
///   "blob <size>\0<raw data>"
///
/// The object ID (hash) is computed over this entire serialized form.
/// This means the same file content always produces the same Blob ID,
/// regardless of filename or location.
class Blob {
public:
    /// Construct a Blob from raw file content.
    explicit Blob(std::string content);

    /// Get the raw content.
    [[nodiscard]] const std::string& content() const { return content_; }

    /// Serialize this Blob into the storage format: "blob <size>\0<data>"
    [[nodiscard]] std::string serialize() const;

    /// Deserialize a Blob from storage format.
    /// Throws MiniGitException if the data is malformed.
    static Blob deserialize(const std::string& data);

private:
    std::string content_;
};

} // namespace minigit
