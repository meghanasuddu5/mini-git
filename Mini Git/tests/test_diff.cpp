/// Tests for the Diff algorithm (Phase 10).

#include "test_utils.h"
#include "minigit/utils/diff.h"

using namespace minigit;

void test_diff_lcs() {
    std::string oldText = "A\nB\nC\nA\nB\nB\nA\n";
    std::string newText = "C\nB\nA\nB\nA\nC\n";

    auto diffs = Diff::compute(oldText, newText);

    // This checks the general behavior of the diff
    // The exact sequence might depend on LCS tie-breaking, but it should contain both
    
    // Let's test a simpler, more deterministic one
    std::string old1 = "line1\nline2\nline3\n";
    std::string new1 = "line1\nline2_mod\nline3\nline4\n";
    
    auto diffs1 = Diff::compute(old1, new1);
    
    ASSERT_EQ(diffs1.size(), 5);
    
    // line1 unchanged
    ASSERT_EQ(diffs1[0].text, "line1");
    ASSERT_TRUE(diffs1[0].type == DiffLine::Type::Unchanged);
    
    // line2 deleted
    ASSERT_EQ(diffs1[1].text, "line2");
    ASSERT_TRUE(diffs1[1].type == DiffLine::Type::Deleted);
    
    // line2_mod added
    ASSERT_EQ(diffs1[2].text, "line2_mod");
    ASSERT_TRUE(diffs1[2].type == DiffLine::Type::Added);
    
    // line3 unchanged
    ASSERT_EQ(diffs1[3].text, "line3");
    ASSERT_TRUE(diffs1[3].type == DiffLine::Type::Unchanged);
    
    // line4 added
    ASSERT_EQ(diffs1[4].text, "line4");
    ASSERT_TRUE(diffs1[4].type == DiffLine::Type::Added);
}

void test_diff_empty() {
    auto diffs = Diff::compute("", "new");
    ASSERT_EQ(diffs.size(), 1);
    ASSERT_TRUE(diffs[0].type == DiffLine::Type::Added);
    ASSERT_EQ(diffs[0].text, "new");

    auto diffs2 = Diff::compute("old", "");
    ASSERT_EQ(diffs2.size(), 1);
    ASSERT_TRUE(diffs2[0].type == DiffLine::Type::Deleted);
    ASSERT_EQ(diffs2[0].text, "old");
}

int main() {
    std::cout << "=== MiniGit Diff Tests ===\n\n";
    RUN_TEST(test_diff_lcs);
    RUN_TEST(test_diff_empty);
    TEST_REPORT();
}
