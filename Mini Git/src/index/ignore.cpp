#include "minigit/index/ignore.h"
#include "minigit/utils/filesystem.h"

#include <fstream>
#include <sstream>

namespace minigit {

Ignore::Ignore(const fs::path& ignoreFilePath) {
    // Always ignore the .minigit directory itself
    patterns_.push_back(std::regex("^\\.minigit(/.*)?$"));

    if (!utils::fileExists(ignoreFilePath)) {
        return;
    }

    std::string content = utils::readFileToString(ignoreFilePath);
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        // Trim whitespace (simple version)
        size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos) continue;
        
        size_t last = line.find_last_not_of(" \t\r");
        std::string pattern = line.substr(first, (last - first + 1));

        // Skip comments and empty lines
        if (pattern.empty() || pattern[0] == '#') {
            continue;
        }

        try {
            std::string regexStr = globToRegex(pattern);
            patterns_.push_back(std::regex(regexStr));
        } catch (...) {
            // If regex compilation fails for some weird pattern, just skip it
            continue;
        }
    }
}

bool Ignore::isIgnored(const std::string& relPath) const {
    for (const auto& pattern : patterns_) {
        if (std::regex_match(relPath, pattern)) {
            return true;
        }
    }
    return false;
}

std::string Ignore::globToRegex(const std::string& pattern) const {
    std::string regex = "^";
    bool isDirectory = false;
    
    std::string p = pattern;

    // If it ends with '/', it's a directory match
    if (p.back() == '/') {
        isDirectory = true;
        p.pop_back();
    }

    // If it doesn't start with '/', it can match anywhere in the tree
    if (p.front() != '/') {
        regex += "(?:.*/)?";
    } else {
        p = p.substr(1); // strip leading slash
    }

    for (char c : p) {
        if (c == '*') {
            regex += "[^/]*"; // '*' matches anything except directory separator
        } else if (c == '.' || c == '+' || c == '?' || c == '(' || c == ')' || 
                   c == '[' || c == ']' || c == '{' || c == '}' || c == '\\' || 
                   c == '^' || c == '$' || c == '|') {
            // Escape regex special characters
            regex += '\\';
            regex += c;
        } else {
            regex += c;
        }
    }

    if (isDirectory) {
        regex += "/.*$"; // Must be followed by / and anything
    } else {
        regex += "(?:/.*)?$"; // Can be the exact file, or a directory (followed by /anything)
    }

    return regex;
}

} // namespace minigit
