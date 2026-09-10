#include "minigit/object/object.h"
#include "minigit/utils/error.h"

#include <sstream>

namespace minigit {

Blob::Blob(std::string content)
    : content_(std::move(content)) {}

std::string Blob::serialize() const {
    // Format: "blob <size>\0<raw data>"
    // The \0 (null byte) separates the header from the content.
    // This is the data that gets hashed to produce the object ID.
    std::string result;
    std::string header = "blob " + std::to_string(content_.size());
    result.reserve(header.size() + 1 + content_.size());
    result.append(header);
    result.push_back('\0');
    result.append(content_);
    return result;
}

Blob Blob::deserialize(const std::string& data) {
    // Find the null byte separating header from content
    size_t nullPos = data.find('\0');
    if (nullPos == std::string::npos) {
        throw MiniGitException("malformed blob object: missing null separator");
    }

    // Parse header: "blob <size>"
    std::string header = data.substr(0, nullPos);
    if (header.substr(0, 5) != "blob ") {
        throw MiniGitException("malformed blob object: invalid header '" + header + "'");
    }

    // Extract and validate size
    size_t declaredSize = 0;
    try {
        declaredSize = std::stoull(header.substr(5));
    } catch (...) {
        throw MiniGitException("malformed blob object: invalid size in header");
    }

    std::string content = data.substr(nullPos + 1);
    if (content.size() != declaredSize) {
        throw MiniGitException(
            "corrupted blob object: declared size " + std::to_string(declaredSize)
            + " but actual size is " + std::to_string(content.size()));
    }

    return Blob(std::move(content));
}

} // namespace minigit
