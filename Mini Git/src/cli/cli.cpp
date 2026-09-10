#include "minigit/cli/cli.h"
#include "minigit/index/index.h"
#include "minigit/object/commit.h"
#include "minigit/object/tree.h"
#include "minigit/object/object_store.h"
#include "minigit/repository/checkout.h"
#include "minigit/repository/graph_analyzer.h"
#include "minigit/repository/merge.h"
#include "minigit/repository/ref_manager.h"
#include "minigit/repository/repository.h"
#include "minigit/utils/diff.h"
#include "minigit/utils/error.h"
#include "minigit/utils/filesystem.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <set>
#include <sstream>

namespace minigit {

namespace {

std::string getAuthor(const Repository& repo) {
    if (const char* envName = std::getenv("MINIGIT_AUTHOR_NAME")) {
        return envName;
    }
    if (const char* envName = std::getenv("GIT_AUTHOR_NAME")) {
        return envName;
    }

    fs::path cfgPath = repo.configPath();
    if (utils::fileExists(cfgPath)) {
        std::string cfg = utils::readFileToString(cfgPath);
        std::istringstream stream(cfg);
        std::string line;
        while (std::getline(stream, line)) {
            size_t pos = line.find("name =");
            if (pos != std::string::npos) {
                std::string name = line.substr(pos + 6);
                size_t first = name.find_first_not_of(" \t\r\n");
                size_t last = name.find_last_not_of(" \t\r\n");
                if (first != std::string::npos && last != std::string::npos) {
                    return name.substr(first, last - first + 1);
                }
            }
        }
    }

    return "MiniGit User";
}

} // anonymous namespace

CLI::CLI(int argc, char* argv[]) {
    programName_ = (argc > 0) ? argv[0] : "minigit";

    if (argc > 1) {
        command_ = argv[1];
    }

    for (int i = 2; i < argc; ++i) {
        args_.emplace_back(argv[i]);
    }
}

int CLI::run() {
    if (command_.empty()) {
        printUsage();
        return 1;
    }

    if (command_ == "--help" || command_ == "-h") {
        printUsage();
        return 0;
    }

    if (command_ == "--version" || command_ == "-v") {
        printVersion();
        return 0;
    }

    try {
        if (command_ == "init")     return handleInit();
        if (command_ == "add")      return handleAdd();
        if (command_ == "status")   return handleStatus();
        if (command_ == "commit")   return handleCommit();
        if (command_ == "log")      return handleLog();
        if (command_ == "show")     return handleShow();
        if (command_ == "branch")   return handleBranch();
        if (command_ == "checkout") return handleCheckout();
        if (command_ == "diff")     return handleDiff();
        if (command_ == "merge")    return handleMerge();

        std::cerr << "minigit: '" << command_
                  << "' is not a minigit command. See 'minigit --help'.\n";
        return 1;

    } catch (const MiniGitException& e) {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "fatal: unexpected error: " << e.what() << "\n";
        return 1;
    }
}

int CLI::handleInit() {
    fs::path targetPath = fs::current_path();

    if (!args_.empty()) {
        targetPath = fs::path(args_[0]);
        if (!fs::exists(targetPath)) {
            fs::create_directories(targetPath);
        }
    }

    Repository repo = Repository::init(targetPath);
    std::cout << "Initialized empty MiniGit repository in "
              << repo.minigitPath().string() << "\n";

    return 0;
}

int CLI::handleAdd() {
    if (args_.empty()) {
        std::cerr << "usage: minigit add <file>...\n"
                  << "       minigit add .          (stage all files)\n";
        return 1;
    }

    Repository repo = Repository::open(fs::current_path());
    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());

    for (const auto& arg : args_) {
        if (arg == ".") {
            index.addAll();
            std::cout << "Added all files to staging area\n";
        } else {
            fs::path filePath = fs::current_path() / arg;
            index.addFile(filePath);
            std::cout << "Added '" << arg << "'\n";
        }
    }

    return 0;
}

int CLI::handleStatus() {
    Repository repo = Repository::open(fs::current_path());
    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    RefManager refManager(repo.minigitPath());

    auto currentBranch = refManager.getCurrentBranch();
    if (currentBranch.has_value()) {
        std::cout << "On branch " << currentBranch.value() << "\n";
    } else {
        std::cout << "HEAD detached at " << refManager.resolveHead().value_or("unknown").substr(0, 7) << "\n";
    }

    auto statusEntries = index.computeStatus();

    std::vector<StatusEntry> staged, modified, deleted, untracked;
    for (const auto& entry : statusEntries) {
        switch (entry.status) {
            case FileStatus::Staged:    staged.push_back(entry);    break;
            case FileStatus::Modified:  modified.push_back(entry);  break;
            case FileStatus::Deleted:   deleted.push_back(entry);   break;
            case FileStatus::Untracked: untracked.push_back(entry); break;
        }
    }

    if (staged.empty() && modified.empty() && deleted.empty() && untracked.empty()) {
        std::cout << "nothing to commit, working tree clean\n";
        return 0;
    }

    if (!staged.empty()) {
        std::cout << "\nChanges to be committed:\n";
        for (const auto& e : staged) {
            std::cout << "        staged:   " << e.path << "\n";
        }
    }

    if (!modified.empty() || !deleted.empty()) {
        std::cout << "\nChanges not staged for commit:\n";
        for (const auto& e : modified) {
            std::cout << "        modified: " << e.path << "\n";
        }
        for (const auto& e : deleted) {
            std::cout << "        deleted:  " << e.path << "\n";
        }
    }

    if (!untracked.empty()) {
        std::cout << "\nUntracked files:\n";
        for (const auto& e : untracked) {
            std::cout << "        " << e.path << "\n";
        }
    }

    return 0;
}

int CLI::handleCommit() {
    std::string message;

    for (size_t i = 0; i < args_.size(); ++i) {
        if (args_[i] == "-m" && i + 1 < args_.size()) {
            message = args_[i + 1];
            break;
        }
    }

    if (message.empty()) {
        std::cerr << "fatal: no commit message provided (use -m \"message\")\n";
        return 1;
    }

    Repository repo = Repository::open(fs::current_path());
    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());

    if (index.size() == 0) {
        std::cout << "nothing to commit (empty index)\n";
        return 0;
    }

    ObjectStore store(repo.objectsPath());

    // 1. Build the Tree from the index
    TreeBuilder builder;
    for (const auto& [path, entry] : index.entries()) {
        builder.addEntry(path, entry.blobHash);
    }
    std::string treeHash = builder.build(store);

    // 2. Determine the parent commit from HEAD
    RefManager refManager(repo.minigitPath());
    auto parentHash = refManager.resolveHead();

    // 3. Get Author
    std::string author = getAuthor(repo);

    // 4. Create and store the Commit
    Commit commit(treeHash, parentHash, author, message);
    std::string commitHash = store.writeRaw(commit.serialize());

    // 5. Update branch reference or HEAD
    auto currentBranch = refManager.getCurrentBranch();
    if (currentBranch.has_value()) {
        refManager.updateBranch(currentBranch.value(), commitHash);
    } else {
        refManager.updateHeadToCommit(commitHash);
    }

    std::cout << "[" << (parentHash.has_value() ? "commit" : "root-commit") 
              << " " << commitHash.substr(0, 7) << "] " << message << "\n";

    return 0;
}

int CLI::handleLog() {
    bool oneline = false;
    int limit = -1;

    for (size_t i = 0; i < args_.size(); ++i) {
        if (args_[i] == "--oneline") {
            oneline = true;
        } else if (args_[i] == "-n" && i + 1 < args_.size()) {
            limit = std::stoi(args_[i + 1]);
        }
    }

    Repository repo = Repository::open(fs::current_path());
    ObjectStore store(repo.objectsPath());
    RefManager refManager(repo.minigitPath());

    auto headOpt = refManager.resolveHead();
    if (!headOpt.has_value()) {
        std::cerr << "fatal: your current branch does not have any commits yet\n";
        return 1;
    }

    auto history = GraphAnalyzer::getHistory(headOpt.value(), store);
    int count = 0;
    bool first = true;

    for (const auto& commitHash : history) {
        if (limit > 0 && count >= limit) break;
        ++count;

        auto raw = store.readRaw(commitHash);
        if (!raw.has_value()) continue;

        Commit commit = Commit::deserialize(raw.value());

        if (oneline) {
            std::string firstLine = commit.message();
            size_t newlinePos = firstLine.find('\n');
            if (newlinePos != std::string::npos) firstLine = firstLine.substr(0, newlinePos);
            std::cout << "\033[33m" << commitHash.substr(0, 7) << "\033[0m " << firstLine << "\n";
        } else {
            if (!first) std::cout << "\n";
            first = false;

            std::cout << "\033[33mcommit " << commitHash << "\033[0m\n";
            if (commit.parentHashes().size() > 1) {
                std::cout << "Merge:";
                for (const auto& p : commit.parentHashes()) {
                    std::cout << " " << p.substr(0, 7);
                }
                std::cout << "\n";
            }
            std::cout << "Author: " << commit.author() << "\n";
            
            std::time_t time = static_cast<std::time_t>(commit.timestamp());
            std::cout << "Date:   " << std::put_time(std::localtime(&time), "%a %b %d %H:%M:%S %Y %z") << "\n\n";

            std::istringstream msgStream(commit.message());
            std::string line;
            while (std::getline(msgStream, line)) {
                std::cout << "    " << line << "\n";
            }
        }
    }

    return 0;
}

int CLI::handleShow() {
    if (args_.empty()) {
        std::cerr << "usage: minigit show <object_or_ref>\n";
        return 1;
    }

    Repository repo = Repository::open(fs::current_path());
    ObjectStore store(repo.objectsPath());
    RefManager refManager(repo.minigitPath());

    auto resolvedHash = refManager.resolveRef(args_[0]);
    std::string hash = resolvedHash.value_or(args_[0]);

    auto rawOpt = store.readRaw(hash);
    if (!rawOpt.has_value()) {
        std::cerr << "fatal: object " << args_[0] << " not found\n";
        return 1;
    }

    std::string raw = rawOpt.value();
    
    if (raw.starts_with("blob ")) {
        Blob blob = Blob::deserialize(raw);
        std::cout << blob.content();
    } 
    else if (raw.starts_with("tree ")) {
        Tree tree = Tree::deserialize(raw);
        for (const auto& entry : tree.entries()) {
            std::cout << entry.mode << " " 
                      << objectTypeToString(entry.type) << " "
                      << entry.hash << "    " 
                      << entry.name << "\n";
        }
    } 
    else if (raw.starts_with("commit ")) {
        Commit commit = Commit::deserialize(raw);
        std::cout << "\033[33mcommit " << hash << "\033[0m\n";
        std::cout << "tree " << commit.treeHash() << "\n";
        for (const auto& p : commit.parentHashes()) {
            std::cout << "parent " << p << "\n";
        }
        
        std::time_t time = static_cast<std::time_t>(commit.timestamp());
        std::cout << "author " << commit.author() << " " 
                  << std::put_time(std::localtime(&time), "%a %b %d %H:%M:%S %Y %z") << "\n\n";
                  
        std::istringstream msgStream(commit.message());
        std::string line;
        while (std::getline(msgStream, line)) {
            std::cout << "    " << line << "\n";
        }

        // Show diff against parent if available
        if (commit.parentHash().has_value()) {
            auto parentRaw = store.readRaw(commit.parentHash().value());
            if (parentRaw.has_value()) {
                Commit parentCommit = Commit::deserialize(parentRaw.value());
                auto oldFiles = MergeEngine::flattenTree(parentCommit.treeHash(), store);
                auto newFiles = MergeEngine::flattenTree(commit.treeHash(), store);

                std::set<std::string> allFiles;
                for (const auto& kv : oldFiles) allFiles.insert(kv.first);
                for (const auto& kv : newFiles) allFiles.insert(kv.first);

                std::cout << "\n";
                for (const std::string& filePath : allFiles) {
                    std::string oldContent = "";
                    std::string newContent = "";
                    if (oldFiles.count(filePath)) {
                        auto r = store.readRaw(oldFiles.at(filePath));
                        if (r.has_value()) oldContent = Blob::deserialize(r.value()).content();
                    }
                    if (newFiles.count(filePath)) {
                        auto r = store.readRaw(newFiles.at(filePath));
                        if (r.has_value()) newContent = Blob::deserialize(r.value()).content();
                    }

                    if (oldContent != newContent) {
                        std::cout << "\033[1mdiff --git a/" << filePath << " b/" << filePath << "\033[0m\n";
                        auto diffs = Diff::compute(oldContent, newContent);
                        for (const auto& d : diffs) {
                            if (d.type == DiffLine::Type::Added) {
                                std::cout << "\033[32m+ " << d.text << "\033[0m\n";
                            } else if (d.type == DiffLine::Type::Deleted) {
                                std::cout << "\033[31m- " << d.text << "\033[0m\n";
                            } else {
                                std::cout << "  " << d.text << "\n";
                            }
                        }
                    }
                }
            }
        }
    } 
    else {
        std::cerr << "fatal: unknown object type\n";
        return 1;
    }

    return 0;
}

int CLI::handleBranch() {
    Repository repo = Repository::open(fs::current_path());
    RefManager refManager(repo.minigitPath());

    if (args_.empty()) {
        auto branches = refManager.listBranches();
        auto currentBranch = refManager.getCurrentBranch();

        for (const auto& branch : branches) {
            if (currentBranch.has_value() && branch == currentBranch.value()) {
                std::cout << "* \033[32m" << branch << "\033[0m\n";
            } else {
                std::cout << "  " << branch << "\n";
            }
        }
        return 0;
    }

    if (args_[0] == "-d" || args_[0] == "-D") {
        if (args_.size() < 2) {
            std::cerr << "fatal: branch name required for deletion\n";
            return 1;
        }
        refManager.deleteBranch(args_[1]);
        std::cout << "Deleted branch " << args_[1] << "\n";
        return 0;
    }

    if (args_[0] == "-m" && args_.size() >= 3) {
        refManager.renameBranch(args_[1], args_[2]);
        std::cout << "Renamed branch " << args_[1] << " to " << args_[2] << "\n";
        return 0;
    }

    std::string branchName = args_[0];
    std::string targetCommit;

    if (args_.size() >= 2) {
        auto resolved = refManager.resolveRef(args_[1]);
        if (!resolved.has_value()) {
            std::cerr << "fatal: not a valid object name: '" << args_[1] << "'\n";
            return 1;
        }
        targetCommit = resolved.value();
    } else {
        auto currentCommit = refManager.resolveHead();
        if (!currentCommit.has_value()) {
            std::cerr << "fatal: not a valid object name: 'HEAD'\n";
            return 1;
        }
        targetCommit = currentCommit.value();
    }

    refManager.createBranch(branchName, targetCommit);
    return 0;
}

int CLI::handleCheckout() {
    if (args_.empty()) {
        std::cerr << "usage: minigit checkout <branch_name_or_commit>\n";
        return 1;
    }

    std::string target = args_[0];
    Repository repo = Repository::open(fs::current_path());
    RefManager refManager(repo.minigitPath());
    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    Checkout checkout(repo.rootPath(), repo.objectsPath());

    if (refManager.branchExists(target)) {
        auto commitHash = refManager.resolveBranch(target);
        if (!commitHash.has_value()) {
            std::cerr << "fatal: branch '" << target << "' has no commits\n";
            return 1;
        }

        checkout.checkoutCommit(commitHash.value(), index);
        refManager.updateHeadToBranch(target);
        std::cout << "Switched to branch '" << target << "'\n";
        return 0;
    }

    auto resolved = refManager.resolveRef(target);
    if (resolved.has_value()) {
        checkout.checkoutCommit(resolved.value(), index);
        refManager.updateHeadToCommit(resolved.value());
        std::cout << "Note: switching to '" << target << "' (detached HEAD at " << resolved.value().substr(0, 7) << ")\n";
        return 0;
    }

    std::cerr << "fatal: pathspec or branch '" << target << "' did not match any file(s) or ref\n";
    return 1;
}

int CLI::handleDiff() {
    Repository repo = Repository::open(fs::current_path());
    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    ObjectStore store(repo.objectsPath());
    RefManager refManager(repo.minigitPath());

    bool staged = false;
    for (const auto& arg : args_) {
        if (arg == "--staged" || arg == "--cached") staged = true;
    }

    if (staged) {
        auto headCommit = refManager.resolveHead();
        std::map<std::string, std::string> headFiles;
        if (headCommit.has_value()) {
            auto raw = store.readRaw(headCommit.value());
            if (raw.has_value()) {
                Commit commit = Commit::deserialize(raw.value());
                headFiles = MergeEngine::flattenTree(commit.treeHash(), store);
            }
        }

        std::set<std::string> allFiles;
        for (const auto& kv : headFiles) allFiles.insert(kv.first);
        for (const auto& kv : index.entries()) allFiles.insert(kv.first);

        for (const std::string& filePath : allFiles) {
            std::string oldContent = "";
            std::string newContent = "";
            if (headFiles.count(filePath)) {
                auto r = store.readRaw(headFiles.at(filePath));
                if (r.has_value()) oldContent = Blob::deserialize(r.value()).content();
            }
            if (index.contains(filePath)) {
                auto blobHashOpt = index.getBlobHash(filePath);
                if (blobHashOpt.has_value()) {
                    auto r = store.readRaw(blobHashOpt.value());
                    if (r.has_value()) newContent = Blob::deserialize(r.value()).content();
                }
            }

            if (oldContent != newContent) {
                std::cout << "\033[1mdiff --staged " << filePath << "\033[0m\n";
                auto diffs = Diff::compute(oldContent, newContent);
                for (const auto& line : diffs) {
                    if (line.type == DiffLine::Type::Added) {
                        std::cout << "\033[32m+ " << line.text << "\033[0m\n";
                    } else if (line.type == DiffLine::Type::Deleted) {
                        std::cout << "\033[31m- " << line.text << "\033[0m\n";
                    } else {
                        std::cout << "  " << line.text << "\n";
                    }
                }
            }
        }
        return 0;
    }

    auto statuses = index.computeStatus();
    for (const auto& status : statuses) {
        if (status.status == FileStatus::Modified) {
            std::cout << "\033[1mdiff " << status.path << "\033[0m\n";

            auto blobHashOpt = index.getBlobHash(status.path);
            std::string oldContent = "";
            if (blobHashOpt.has_value()) {
                auto rawOpt = store.readRaw(blobHashOpt.value());
                if (rawOpt.has_value()) {
                    Blob blob = Blob::deserialize(rawOpt.value());
                    oldContent = blob.content();
                }
            }

            std::string newContent = utils::readFileToString(repo.rootPath() / status.path);
            auto diffs = Diff::compute(oldContent, newContent);
            
            for (const auto& line : diffs) {
                if (line.type == DiffLine::Type::Added) {
                    std::cout << "\033[32m+ " << line.text << "\033[0m\n";
                } else if (line.type == DiffLine::Type::Deleted) {
                    std::cout << "\033[31m- " << line.text << "\033[0m\n";
                } else {
                    std::cout << "  " << line.text << "\n";
                }
            }
            std::cout << "\n";
        }
    }

    return 0;
}

int CLI::handleMerge() {
    if (args_.empty()) {
        std::cerr << "usage: minigit merge <branch_name>\n";
        return 1;
    }
    
    std::string targetBranch = args_[0];
    
    Repository repo = Repository::open(fs::current_path());
    RefManager refManager(repo.minigitPath());
    ObjectStore store(repo.objectsPath());

    auto currentBranch = refManager.getCurrentBranch();
    if (!currentBranch.has_value()) {
        std::cerr << "fatal: you are not on a branch. Cannot merge.\n";
        return 1;
    }
    
    auto currentCommitOpt = refManager.resolveHead();
    if (!currentCommitOpt.has_value()) {
        std::cerr << "fatal: current branch does not have any commits yet\n";
        return 1;
    }
    std::string currentCommit = currentCommitOpt.value();
    
    auto targetCommitOpt = refManager.resolveBranch(targetBranch);
    if (!targetCommitOpt.has_value()) {
        std::cerr << "fatal: branch '" << targetBranch << "' not found\n";
        return 1;
    }
    std::string targetCommit = targetCommitOpt.value();
    
    if (currentCommit == targetCommit) {
        std::cout << "Already up to date.\n";
        return 0;
    }

    // 1. Check if fast-forward is possible (current is ancestor of target)
    if (GraphAnalyzer::isAncestor(currentCommit, targetCommit, store)) {
        std::cout << "Updating " << currentCommit.substr(0, 7) << ".." << targetCommit.substr(0, 7) << "\n";
        std::cout << "Fast-forward\n";
        
        Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
        Checkout checkout(repo.rootPath(), repo.objectsPath());
        checkout.checkoutCommit(targetCommit, index);
        refManager.updateBranch(currentBranch.value(), targetCommit);
        return 0;
    }

    // 2. Perform 3-way merge
    auto mergeBaseOpt = GraphAnalyzer::findMergeBase(currentCommit, targetCommit, store);
    std::string baseTreeHash = "";
    if (mergeBaseOpt.has_value()) {
        auto rawBase = store.readRaw(mergeBaseOpt.value());
        if (rawBase.has_value()) {
            baseTreeHash = Commit::deserialize(rawBase.value()).treeHash();
        }
    }

    auto rawOurs = store.readRaw(currentCommit);
    auto rawTheirs = store.readRaw(targetCommit);
    if (!rawOurs.has_value() || !rawTheirs.has_value()) {
        std::cerr << "fatal: corrupted commits during merge\n";
        return 1;
    }

    std::string oursTreeHash = Commit::deserialize(rawOurs.value()).treeHash();
    std::string theirsTreeHash = Commit::deserialize(rawTheirs.value()).treeHash();

    std::cout << "Auto-merging branch '" << targetBranch << "' into '" << currentBranch.value() << "'\n";

    MergeResult mergeResult = MergeEngine::mergeTrees(
        baseTreeHash, oursTreeHash, theirsTreeHash, store, currentBranch.value(), targetBranch);

    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());

    if (mergeResult.hasConflicts) {
        // Write conflicting files with conflict markers to working tree
        for (const auto& [path, content] : mergeResult.fileContents) {
            fs::path absPath = repo.rootPath() / path;
            fs::create_directories(absPath.parent_path());
            utils::writeStringToFile(absPath, content);
        }

        std::cerr << "CONFLICT: Automatic merge failed; fix conflicts and then commit the result.\n";
        for (const auto& cf : mergeResult.conflictedFiles) {
            std::cerr << "Auto-merging " << cf << " (conflict)\n";
        }
        return 1;
    }

    // Clean 3-way merge!
    // Write all merged files to disk and update index
    for (const auto& [path, content] : mergeResult.fileContents) {
        fs::path absPath = repo.rootPath() / path;
        fs::create_directories(absPath.parent_path());
        utils::writeStringToFile(absPath, content);
    }
    for (const auto& [path, blobHash] : mergeResult.fileBlobs) {
        index.setEntry(path, blobHash);
    }
    for (const auto& delPath : mergeResult.deletedFiles) {
        fs::path absPath = repo.rootPath() / delPath;
        if (utils::fileExists(absPath)) fs::remove(absPath);
        if (index.contains(delPath)) index.removeFile(delPath);
    }

    // Build new tree
    TreeBuilder builder;
    for (const auto& [path, entry] : index.entries()) {
        builder.addEntry(path, entry.blobHash);
    }
    std::string mergedTree = builder.build(store);

    // Create 2-parent merge commit
    std::string author = getAuthor(repo);
    std::string msg = "Merge branch '" + targetBranch + "' into " + currentBranch.value();
    Commit mergeCommit(mergedTree, {currentCommit, targetCommit}, author, msg);
    std::string mergeCommitHash = store.writeRaw(mergeCommit.serialize());

    // Update branch pointer
    refManager.updateBranch(currentBranch.value(), mergeCommitHash);

    std::cout << "Merge made by the 3-way merge strategy.\n";
    std::cout << "[" << currentBranch.value() << " " << mergeCommitHash.substr(0, 7) << "] " << msg << "\n";

    return 0;
}

void CLI::printUsage() const {
    std::cout << "usage: minigit <command> [<args>]\n"
              << "\n"
              << "Available commands:\n"
              << "   init          Create a new MiniGit repository\n"
              << "   add           Stage file(s) for commit\n"
              << "   status        Show working tree status\n"
              << "   commit        Record changes to the repository\n"
              << "   log           Show commit history\n"
              << "   show          Show commit details and diff\n"
              << "   diff          Show changes between commits or working tree\n"
              << "   checkout      Restore working tree or switch branches\n"
              << "   branch        List, create, or delete branches\n"
              << "   merge         Merge a branch into the current branch\n"
              << "\n"
              << "Options:\n"
              << "   --help, -h    Show this help message\n"
              << "   --version, -v Show version information\n";
}

void CLI::printVersion() const {
    std::cout << "minigit version 0.1.0\n";
}

} // namespace minigit
