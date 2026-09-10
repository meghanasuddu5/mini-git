#pragma once

#include <string>
#include <vector>

namespace minigit {

/// Represents a single line in a diff.
struct DiffLine {
    enum class Type {
        Unchanged,
        Added,
        Deleted
    };

    Type type;
    std::string text;

    bool operator==(const DiffLine& other) const {
        return type == other.type && text == other.text;
    }
};

class Diff {
public:
    /// Compute the line-by-line difference between two strings using the
    /// Longest Common Subsequence (LCS) dynamic programming algorithm.
    ///
    /// This is a classic O(N*M) algorithm that generates a list of
    /// insertions (Added), deletions (Deleted), and matching lines (Unchanged).
    ///
    /// @param oldContent The original text.
    /// @param newContent The new text.
    /// @return A list of DiffLine objects representing the diff sequence.
    static std::vector<DiffLine> compute(const std::string& oldContent, 
                                         const std::string& newContent);
};

} // namespace minigit
