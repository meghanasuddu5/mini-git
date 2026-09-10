#include "minigit/object/commit.h"
#include "minigit/utils/error.h"

#include <chrono>
#include <sstream>

namespace minigit {

Commit::Commit(std::string treeHash,
               std::vector<std::string> parentHashes,
               std::string author,
               uint64_t timestamp,
               std::string message)
    : treeHash_(std::move(treeHash))
    , parentHashes_(std::move(parentHashes))
    , author_(std::move(author))
    , timestamp_(timestamp)
    , message_(std::move(message)) {}

Commit::Commit(std::string treeHash,
               std::vector<std::string> parentHashes,
               std::string author,
               std::string message)
    : treeHash_(std::move(treeHash))
    , parentHashes_(std::move(parentHashes))
    , author_(std::move(author))
    , message_(std::move(message)) {
    auto now = std::chrono::system_clock::now();
    timestamp_ = std::chrono::duration_cast<std::chrono::seconds>(
                     now.time_since_epoch())
                     .count();
}

Commit::Commit(std::string treeHash,
               std::optional<std::string> parentHash,
               std::string author,
               uint64_t timestamp,
               std::string message)
    : treeHash_(std::move(treeHash))
    , author_(std::move(author))
    , timestamp_(timestamp)
    , message_(std::move(message)) {
    if (parentHash.has_value()) {
        parentHashes_.push_back(std::move(parentHash.value()));
    }
}

Commit::Commit(std::string treeHash,
               std::optional<std::string> parentHash,
               std::string author,
               std::string message)
    : treeHash_(std::move(treeHash))
    , author_(std::move(author))
    , message_(std::move(message)) {
    if (parentHash.has_value()) {
        parentHashes_.push_back(std::move(parentHash.value()));
    }
    auto now = std::chrono::system_clock::now();
    timestamp_ = std::chrono::duration_cast<std::chrono::seconds>(
                     now.time_since_epoch())
                     .count();
}

std::string Commit::serialize() const {
    std::ostringstream body;
    body << "tree " << treeHash_ << "\n";
    for (const auto& parent : parentHashes_) {
        body << "parent " << parent << "\n";
    }
    body << "author " << author_ << " " << timestamp_ << "\n";
    body << "\n";
    body << message_ << "\n";

    std::string bodyStr = body.str();
    std::ostringstream result;
    result << "commit " << bodyStr.size() << '\0' << bodyStr;
    return result.str();
}

Commit Commit::deserialize(const std::string& data) {
    size_t nullPos = data.find('\0');
    if (nullPos == std::string::npos) {
        throw MiniGitException("malformed commit object: missing null separator");
    }

    std::string header = data.substr(0, nullPos);
    if (header.substr(0, 7) != "commit ") {
        throw MiniGitException("malformed commit object: invalid header '" + header + "'");
    }

    std::string body = data.substr(nullPos + 1);
    std::istringstream stream(body);
    std::string line;

    std::string treeHash;
    std::vector<std::string> parentHashes;
    std::string author;
    uint64_t timestamp = 0;
    std::string message;

    while (std::getline(stream, line)) {
        if (line.empty()) {
            // Empty line marks the end of headers and start of message
            break;
        }

        if (line.starts_with("tree ")) {
            treeHash = line.substr(5);
        } else if (line.starts_with("parent ")) {
            parentHashes.push_back(line.substr(7));
        } else if (line.starts_with("author ")) {
            std::string authorInfo = line.substr(7);
            size_t lastSpace = authorInfo.find_last_of(' ');
            if (lastSpace == std::string::npos) {
                throw MiniGitException("malformed commit object: invalid author line");
            }
            author = authorInfo.substr(0, lastSpace);
            try {
                timestamp = std::stoull(authorInfo.substr(lastSpace + 1));
            } catch (...) {
                throw MiniGitException("malformed commit object: invalid timestamp");
            }
        }
    }
    
    // The rest of the stream is the message
    std::ostringstream msgStream;
    msgStream << stream.rdbuf();
    message = msgStream.str();

    // Strip trailing newline added by serialize()
    if (!message.empty() && message.back() == '\n') {
        message.pop_back();
    }

    if (treeHash.empty() || author.empty() || message.empty()) {
        throw MiniGitException("malformed commit object: missing required fields");
    }

    return Commit(treeHash, parentHashes, author, timestamp, message);
}

} // namespace minigit
