#include "minigit/repository/merge.h"
#include "minigit/object/object.h"
#include "minigit/object/tree.h"
#include "minigit/utils/diff.h"
#include "minigit/utils/error.h"

#include <algorithm>
#include <set>
#include <sstream>

namespace minigit {

namespace {

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    if (text.empty()) {
        return lines;
    }
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::string joinLines(const std::vector<std::string>& lines) {
    std::string result;
    for (const auto& line : lines) {
        result += line + "\n";
    }
    return result;
}

} // anonymous namespace

void MergeEngine::flattenTreeRecursive(
    const std::string& treeHash,
    const std::string& currentPath,
    const ObjectStore& store,
    std::map<std::string, std::string>& outFiles) {

    if (treeHash.empty()) {
        return;
    }

    auto rawOpt = store.readRaw(treeHash);
    if (!rawOpt.has_value()) {
        return;
    }

    Tree tree = Tree::deserialize(rawOpt.value());
    for (const auto& entry : tree.entries()) {
        std::string relPath = currentPath.empty() ? entry.name : currentPath + "/" + entry.name;
        if (entry.type == ObjectType::Blob) {
            outFiles[relPath] = entry.hash;
        } else if (entry.type == ObjectType::Tree) {
            flattenTreeRecursive(entry.hash, relPath, store, outFiles);
        }
    }
}

std::map<std::string, std::string> MergeEngine::flattenTree(
    const std::string& treeHash,
    const ObjectStore& store) {

    std::map<std::string, std::string> files;
    flattenTreeRecursive(treeHash, "", store, files);
    return files;
}

std::pair<std::string, bool> MergeEngine::merge3WayLines(
    const std::string& baseContent,
    const std::string& oursContent,
    const std::string& theirsContent,
    const std::string& ourBranchName,
    const std::string& theirBranchName) {

    // Fast paths
    if (oursContent == theirsContent) {
        return {oursContent, false};
    }
    if (oursContent == baseContent) {
        return {theirsContent, false};
    }
    if (theirsContent == baseContent) {
        return {oursContent, false};
    }

    // Line-by-line 3-way merge
    // If base is empty, both created different versions of the file
    if (baseContent.empty()) {
        std::string conflict;
        conflict += "<<<<<<< " + ourBranchName + "\n";
        conflict += oursContent;
        if (!oursContent.empty() && oursContent.back() != '\n') conflict += "\n";
        conflict += "=======\n";
        conflict += theirsContent;
        if (!theirsContent.empty() && theirsContent.back() != '\n') conflict += "\n";
        conflict += ">>>>>>> " + theirBranchName + "\n";
        return {conflict, true};
    }

    auto baseLines = splitLines(baseContent);
    auto oursLines = splitLines(oursContent);
    auto theirsLines = splitLines(theirsContent);

    // Compute diffs against base
    auto diffOurs = Diff::compute(baseContent, oursContent);
    auto diffTheirs = Diff::compute(baseContent, theirsContent);

    // Structure changes relative to base line indices
    struct Change {
        std::vector<std::string> deletedBaseLines;
        std::vector<std::string> addedLines;
    };

    // Simple chunk-based 3-way merge
    // If both modified the file in mutually incompatible ways, generate conflict markers
    std::string result;
    bool hasConflict = false;

    // Check line by line if both made different edits
    // For general text, if diffs don't trivially commute, we format conflict markers for the entire divergence
    // or line-by-line merge.
    
    // Line diffs alignment:
    size_t iBase = 0, iOurs = 0, iTheirs = 0;
    while (iBase < baseLines.size() || iOurs < oursLines.size() || iTheirs < theirsLines.size()) {
        // If all match
        if (iBase < baseLines.size() && iOurs < oursLines.size() && iTheirs < theirsLines.size()
            && baseLines[iBase] == oursLines[iOurs] && baseLines[iBase] == theirsLines[iTheirs]) {
            result += baseLines[iBase] + "\n";
            ++iBase;
            ++iOurs;
            ++iTheirs;
            continue;
        }

        // If OURS matches BASE, but THEIRS changed
        if (iBase < baseLines.size() && iOurs < oursLines.size() && baseLines[iBase] == oursLines[iOurs]) {
            if (iTheirs < theirsLines.size()) {
                result += theirsLines[iTheirs] + "\n";
                ++iTheirs;
            }
            ++iBase;
            ++iOurs;
            continue;
        }

        // If THEIRS matches BASE, but OURS changed
        if (iBase < baseLines.size() && iTheirs < theirsLines.size() && baseLines[iBase] == theirsLines[iTheirs]) {
            if (iOurs < oursLines.size()) {
                result += oursLines[iOurs] + "\n";
                ++iOurs;
            }
            ++iBase;
            ++iTheirs;
            continue;
        }

        // If both changed identically
        if (iOurs < oursLines.size() && iTheirs < theirsLines.size() && oursLines[iOurs] == theirsLines[iTheirs]) {
            result += oursLines[iOurs] + "\n";
            if (iBase < baseLines.size()) ++iBase;
            ++iOurs;
            ++iTheirs;
            continue;
        }

        // Conflicting region
        hasConflict = true;
        result += "<<<<<<< " + ourBranchName + "\n";
        while (iOurs < oursLines.size() && (iBase >= baseLines.size() || oursLines[iOurs] != baseLines[iBase])) {
            result += oursLines[iOurs] + "\n";
            ++iOurs;
        }
        result += "=======\n";
        while (iTheirs < theirsLines.size() && (iBase >= baseLines.size() || theirsLines[iTheirs] != baseLines[iBase])) {
            result += theirsLines[iTheirs] + "\n";
            ++iTheirs;
        }
        result += ">>>>>>> " + theirBranchName + "\n";

        if (iBase < baseLines.size()) {
            ++iBase;
        }
    }

    return {result, hasConflict};
}

MergeResult MergeEngine::mergeTrees(
    const std::string& baseTreeHash,
    const std::string& oursTreeHash,
    const std::string& theirsTreeHash,
    ObjectStore& store,
    const std::string& ourBranchName,
    const std::string& theirBranchName) {

    MergeResult result;

    auto baseFiles = flattenTree(baseTreeHash, store);
    auto oursFiles = flattenTree(oursTreeHash, store);
    auto theirsFiles = flattenTree(theirsTreeHash, store);

    // Collect all paths
    std::set<std::string> allPaths;
    for (const auto& [path, _] : baseFiles) allPaths.insert(path);
    for (const auto& [path, _] : oursFiles) allPaths.insert(path);
    for (const auto& [path, _] : theirsFiles) allPaths.insert(path);

    for (const auto& path : allPaths) {
        bool inBase = baseFiles.find(path) != baseFiles.end();
        bool inOurs = oursFiles.find(path) != oursFiles.end();
        bool inTheirs = theirsFiles.find(path) != theirsFiles.end();

        std::string baseHash = inBase ? baseFiles[path] : "";
        std::string oursHash = inOurs ? oursFiles[path] : "";
        std::string theirsHash = inTheirs ? theirsFiles[path] : "";

        // Case 1: Unchanged or identical in both branches
        if (inOurs && inTheirs && oursHash == theirsHash) {
            result.fileBlobs[path] = oursHash;
            result.mergedFiles.push_back(path);
            continue;
        }

        // Case 2: Changed only in OURS (THEIRS matches base or base is empty and only OURS added)
        if (inOurs && inBase && inTheirs && theirsHash == baseHash && oursHash != baseHash) {
            result.fileBlobs[path] = oursHash;
            result.mergedFiles.push_back(path);
            continue;
        }
        if (inOurs && !inBase && !inTheirs) {
            result.fileBlobs[path] = oursHash;
            result.mergedFiles.push_back(path);
            continue;
        }

        // Case 3: Changed only in THEIRS (OURS matches base or base is empty and only THEIRS added)
        if (inTheirs && inBase && inOurs && oursHash == baseHash && theirsHash != baseHash) {
            result.fileBlobs[path] = theirsHash;
            result.mergedFiles.push_back(path);
            continue;
        }
        if (inTheirs && !inBase && !inOurs) {
            result.fileBlobs[path] = theirsHash;
            result.mergedFiles.push_back(path);
            continue;
        }

        // Case 4: Deleted in THEIRS, unchanged in OURS
        if (!inTheirs && inBase && inOurs && oursHash == baseHash) {
            result.deletedFiles.push_back(path);
            continue;
        }

        // Case 5: Deleted in OURS, unchanged in THEIRS
        if (!inOurs && inBase && inTheirs && theirsHash == baseHash) {
            result.deletedFiles.push_back(path);
            continue;
        }

        // Case 6: Deleted in both
        if (!inOurs && !inTheirs && inBase) {
            result.deletedFiles.push_back(path);
            continue;
        }

        // Case 7: Content conflict / modification in both branches
        std::string baseContent = "";
        if (inBase) {
            auto raw = store.readRaw(baseHash);
            if (raw.has_value()) {
                baseContent = Blob::deserialize(raw.value()).content();
            }
        }

        std::string oursContent = "";
        if (inOurs) {
            auto raw = store.readRaw(oursHash);
            if (raw.has_value()) {
                oursContent = Blob::deserialize(raw.value()).content();
            }
        }

        std::string theirsContent = "";
        if (inTheirs) {
            auto raw = store.readRaw(theirsHash);
            if (raw.has_value()) {
                theirsContent = Blob::deserialize(raw.value()).content();
            }
        }

        auto [mergedText, conflict] = merge3WayLines(
            baseContent, oursContent, theirsContent, ourBranchName, theirBranchName);

        if (conflict) {
            result.hasConflicts = true;
            result.conflictedFiles.push_back(path);
            result.fileContents[path] = mergedText;
        } else {
            // Clean line merge! Write new blob to object store
            Blob newBlob(mergedText);
            std::string newHash = store.writeBlob(newBlob);
            result.fileBlobs[path] = newHash;
            result.fileContents[path] = mergedText;
            result.mergedFiles.push_back(path);
        }
    }

    return result;
}

} // namespace minigit
