#include "minigit/repository/ref_manager.h"
#include "minigit/utils/error.h"
#include "minigit/utils/filesystem.h"

#include <algorithm>

namespace minigit {

RefManager::RefManager(const fs::path& minigitDir)
    : minigitDir_(minigitDir)
    , headPath_(minigitDir / "HEAD")
    , refsHeadsDir_(minigitDir / "refs" / "heads") {}

// ─── HEAD Operations ────────────────────────────────────────────────────────

std::optional<std::string> RefManager::resolveHead() const {
    if (!utils::fileExists(headPath_)) {
        return std::nullopt;
    }

    std::string headContent = utils::readFileToString(headPath_);
    if (!headContent.empty() && headContent.back() == '\n') {
        headContent.pop_back();
    }

    if (headContent.starts_with("ref: ")) {
        // e.g., "ref: refs/heads/main"
        std::string refPath = headContent.substr(5);
        fs::path fullRefPath = minigitDir_ / refPath;
        
        if (!utils::fileExists(fullRefPath)) {
            return std::nullopt; // Branch doesn't exist yet (e.g. initial state)
        }
        
        std::string hash = utils::readFileToString(fullRefPath);
        if (!hash.empty() && hash.back() == '\n') hash.pop_back();
        return hash;
    } else {
        // Detached HEAD
        return headContent;
    }
}

std::string RefManager::readHead() const {
    if (!utils::fileExists(headPath_)) {
        throw MiniGitException("HEAD file is missing");
    }
    std::string content = utils::readFileToString(headPath_);
    if (!content.empty() && content.back() == '\n') content.pop_back();
    return content;
}

void RefManager::updateHeadToBranch(const std::string& branchName) {
    std::string content = "ref: refs/heads/" + branchName + "\n";
    utils::writeStringToFile(headPath_, content);
}

void RefManager::updateHeadToCommit(const std::string& commitHash) {
    std::string content = commitHash + "\n";
    utils::writeStringToFile(headPath_, content);
}

// ─── Branch Operations ──────────────────────────────────────────────────────

std::vector<std::string> RefManager::listBranches() const {
    std::vector<std::string> branches;
    if (!fs::exists(refsHeadsDir_)) {
        return branches;
    }

    for (const auto& entry : fs::directory_iterator(refsHeadsDir_)) {
        if (entry.is_regular_file()) {
            branches.push_back(entry.path().filename().string());
        }
    }

    std::sort(branches.begin(), branches.end());
    return branches;
}

bool RefManager::branchExists(const std::string& branchName) const {
    return utils::fileExists(refsHeadsDir_ / branchName);
}

void RefManager::createBranch(const std::string& branchName, const std::string& commitHash) {
    if (branchExists(branchName)) {
        throw MiniGitException("branch '" + branchName + "' already exists");
    }

    if (!fs::exists(refsHeadsDir_)) {
        fs::create_directories(refsHeadsDir_);
    }

    utils::writeStringToFile(refsHeadsDir_ / branchName, commitHash + "\n");
}

std::optional<std::string> RefManager::resolveBranch(const std::string& branchName) const {
    fs::path branchPath = refsHeadsDir_ / branchName;
    if (!utils::fileExists(branchPath)) {
        return std::nullopt;
    }

    std::string hash = utils::readFileToString(branchPath);
    if (!hash.empty() && hash.back() == '\n') hash.pop_back();
    return hash;
}

void RefManager::updateBranch(const std::string& branchName, const std::string& commitHash) {
    if (!fs::exists(refsHeadsDir_)) {
        fs::create_directories(refsHeadsDir_);
    }

    utils::writeStringToFile(refsHeadsDir_ / branchName, commitHash + "\n");
}

void RefManager::deleteBranch(const std::string& branchName) {
    if (!branchExists(branchName)) {
        throw MiniGitException("branch '" + branchName + "' not found.");
    }

    auto current = getCurrentBranch();
    if (current.has_value() && current.value() == branchName) {
        throw MiniGitException("cannot delete branch '" + branchName + "' checked out at '" + minigitDir_.parent_path().string() + "'");
    }

    fs::remove(refsHeadsDir_ / branchName);
}

void RefManager::renameBranch(const std::string& oldName, const std::string& newName) {
    if (!branchExists(oldName)) {
        throw MiniGitException("branch '" + oldName + "' does not exist");
    }
    if (branchExists(newName)) {
        throw MiniGitException("a branch named '" + newName + "' already exists");
    }

    auto commitHash = resolveBranch(oldName);
    if (!commitHash.has_value()) {
        throw MiniGitException("corrupted branch ref: " + oldName);
    }

    createBranch(newName, commitHash.value());
    
    auto current = getCurrentBranch();
    if (current.has_value() && current.value() == oldName) {
        updateHeadToBranch(newName);
    }

    fs::remove(refsHeadsDir_ / oldName);
}

std::optional<std::string> RefManager::getCurrentBranch() const {
    if (!utils::fileExists(headPath_)) {
        return std::nullopt;
    }
    std::string content = readHead();
    if (content.starts_with("ref: refs/heads/")) {
        return content.substr(16);
    }
    return std::nullopt;
}

std::optional<std::string> RefManager::resolveRef(const std::string& refOrHash) const {
    if (refOrHash.empty()) {
        return std::nullopt;
    }

    if (refOrHash == "HEAD") {
        return resolveHead();
    }

    if (branchExists(refOrHash)) {
        return resolveBranch(refOrHash);
    }

    // Direct hash check (64 hex characters or prefix search in objects)
    if (refOrHash.size() == 64) {
        return refOrHash;
    }

    // Short hash prefix search (>= 4 chars)
    if (refOrHash.size() >= 4 && refOrHash.size() < 64) {
        fs::path objectsDir = minigitDir_ / "objects";
        std::string prefix = refOrHash.substr(0, 2);
        std::string suffix = refOrHash.substr(2);
        fs::path dir = objectsDir / prefix;
        if (fs::exists(dir)) {
            std::vector<std::string> matches;
            for (const auto& entry : fs::directory_iterator(dir)) {
                std::string filename = entry.path().filename().string();
                if (filename.starts_with(suffix)) {
                    matches.push_back(prefix + filename);
                }
            }
            if (matches.size() == 1) {
                return matches[0];
            }
        }
    }

    return std::nullopt;
}

} // namespace minigit
