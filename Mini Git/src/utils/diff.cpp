#include "minigit/utils/diff.h"

#include <algorithm>
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
    // std::getline strips '\n'.
    // If the file doesn't end with '\n', the last line will still be read.
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

} // anonymous namespace

std::vector<DiffLine> Diff::compute(const std::string& oldContent, 
                                    const std::string& newContent) {
    std::vector<std::string> oldLines = splitLines(oldContent);
    std::vector<std::string> newLines = splitLines(newContent);

    int m = static_cast<int>(oldLines.size());
    int n = static_cast<int>(newLines.size());

    // LCS DP table. We only need simple 2D vector for O(N*M) space.
    // For large files this isn't optimal, but it is perfect for a portfolio project
    // to demonstrate understanding of dynamic programming.
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));

    // Fill the DP table
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (oldLines[i - 1] == newLines[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    // Backtrack to find the diff sequence
    std::vector<DiffLine> diffs;
    int i = m, j = n;
    
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && oldLines[i - 1] == newLines[j - 1]) {
            // Unchanged line
            diffs.push_back({DiffLine::Type::Unchanged, oldLines[i - 1]});
            --i;
            --j;
        } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
            // Added line (came from left in DP table)
            diffs.push_back({DiffLine::Type::Added, newLines[j - 1]});
            --j;
        } else if (i > 0 && (j == 0 || dp[i][j - 1] < dp[i - 1][j])) {
            // Deleted line (came from top in DP table)
            diffs.push_back({DiffLine::Type::Deleted, oldLines[i - 1]});
            --i;
        }
    }

    // Backtracking builds the diff in reverse order, so we reverse it
    std::reverse(diffs.begin(), diffs.end());

    return diffs;
}

} // namespace minigit
