#include "minigit/object/tree.h"
#include "minigit/object/hash.h"
#include "minigit/object/object_store.h"
#include "minigit/utils/error.h"

#include <algorithm>
#include <sstream>

namespace minigit {

// ─── Tree ───────────────────────────────────────────────────────────────────

Tree::Tree(std::vector<TreeEntry> entries)
    : entries_(std::move(entries)) {
    // Sort by name for deterministic hashing.
    // This is critical: the same set of files must always produce
    // the same tree hash, regardless of insertion order.
    std::sort(entries_.begin(), entries_.end(),
              [](const TreeEntry& a, const TreeEntry& b) {
                  return a.name < b.name;
              });
}

std::string Tree::serialize() const {
    // Build the body first: each entry is one line
    //   "<mode> <type> <hash> <name>\n"
    std::string body;
    for (const auto& entry : entries_) {
        body += entry.mode + " "
              + std::string(objectTypeToString(entry.type)) + " "
              + entry.hash + " "
              + entry.name + "\n";
    }

    // Full format: "tree <body_size>\0<body>"
    std::string header = "tree " + std::to_string(body.size());
    std::string result;
    result.reserve(header.size() + 1 + body.size());
    result.append(header);
    result.push_back('\0');
    result.append(body);
    return result;
}

Tree Tree::deserialize(const std::string& data) {
    // Find the null separator
    size_t nullPos = data.find('\0');
    if (nullPos == std::string::npos) {
        throw MiniGitException("malformed tree object: missing null separator");
    }

    // Validate header
    std::string header = data.substr(0, nullPos);
    if (header.substr(0, 5) != "tree ") {
        throw MiniGitException("malformed tree object: invalid header '" + header + "'");
    }

    // Parse entries from body
    std::string body = data.substr(nullPos + 1);
    std::vector<TreeEntry> entries;
    std::istringstream stream(body);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        // Parse: "<mode> <type> <hash> <name>"
        // We split on spaces, but the name might theoretically contain spaces
        // For simplicity, we parse the first 3 space-separated tokens,
        // and everything after the third space is the name.
        std::istringstream lineStream(line);
        TreeEntry entry;

        if (!(lineStream >> entry.mode)) {
            throw MiniGitException("malformed tree entry: missing mode");
        }

        std::string typeStr;
        if (!(lineStream >> typeStr)) {
            throw MiniGitException("malformed tree entry: missing type");
        }
        entry.type = stringToObjectType(typeStr);

        if (!(lineStream >> entry.hash)) {
            throw MiniGitException("malformed tree entry: missing hash");
        }

        // The rest of the line is the name
        std::getline(lineStream >> std::ws, entry.name);
        if (entry.name.empty()) {
            throw MiniGitException("malformed tree entry: missing name");
        }

        entries.push_back(std::move(entry));
    }

    return Tree(std::move(entries));
}

const TreeEntry* Tree::findEntry(const std::string& name) const {
    for (const auto& entry : entries_) {
        if (entry.name == name) {
            return &entry;
        }
    }
    return nullptr;
}

// ─── TreeBuilder ────────────────────────────────────────────────────────────

void TreeBuilder::addEntry(const std::string& path, const std::string& blobHash) {
    // Split the path into components.
    // Example: "src/utils/help.h" → ["src", "utils", "help.h"]
    //
    // We navigate the DirNode tree, creating subdirectory nodes as needed,
    // and add the file to the deepest directory.

    DirNode* current = &root_;
    fs::path p(path);

    // Collect path components
    std::vector<std::string> components;
    for (const auto& part : p) {
        components.push_back(part.string());
    }

    // Navigate to the correct directory, creating nodes as needed
    for (size_t i = 0; i < components.size() - 1; ++i) {
        current = &current->subdirs[components[i]];
    }

    // Add the file to the final directory
    current->files[components.back()] = blobHash;
}

std::string TreeBuilder::build(ObjectStore& store) {
    if (root_.files.empty() && root_.subdirs.empty()) {
        throw MiniGitException("cannot build tree: no entries");
    }
    return buildNode(root_, store);
}

std::string TreeBuilder::buildNode(DirNode& node, ObjectStore& store) {
    std::vector<TreeEntry> entries;

    // Add file entries (blobs)
    for (const auto& [name, hash] : node.files) {
        entries.push_back(TreeEntry{
            "100644",           // Regular file mode
            ObjectType::Blob,
            hash,
            name
        });
    }

    // Recursively build subdirectory trees (bottom-up!)
    // We must hash child trees BEFORE we can hash the parent,
    // because the parent's hash depends on children's hashes.
    for (auto& [name, childNode] : node.subdirs) {
        std::string childHash = buildNode(childNode, store);
        entries.push_back(TreeEntry{
            "040000",           // Directory mode
            ObjectType::Tree,
            childHash,
            name
        });
    }

    // Create the Tree object and store it
    Tree tree(std::move(entries));
    std::string serialized = tree.serialize();
    return store.writeRaw(serialized);
}

} // namespace minigit
