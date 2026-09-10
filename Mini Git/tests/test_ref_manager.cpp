/// Tests for RefManager (Phase 9).
///
/// Test plan:
///   1. Resolve HEAD (initial, detached, branch)
///   2. Create branch
///   3. List branches
///   4. Update HEAD

#include "test_utils.h"
#include "minigit/repository/ref_manager.h"
#include "minigit/utils/filesystem.h"
#include "minigit/utils/error.h"

using namespace minigit;

void test_ref_manager_operations() {
    TempDir tmp;
    fs::path minigitDir = tmp.path() / ".minigit";
    fs::create_directories(minigitDir / "refs" / "heads");

    RefManager refs(minigitDir);

    // Test initial HEAD (points to non-existent main branch)
    utils::writeStringToFile(minigitDir / "HEAD", "ref: refs/heads/main\n");
    ASSERT_FALSE(refs.resolveHead().has_value());
    
    // Create commit hash
    std::string commit1 = std::string(64, '1');
    std::string commit2 = std::string(64, '2');

    // Create branch 'main'
    refs.createBranch("main", commit1);
    ASSERT_TRUE(refs.branchExists("main"));
    ASSERT_EQ(refs.resolveBranch("main").value(), commit1);
    
    // Now HEAD should resolve
    ASSERT_EQ(refs.resolveHead().value(), commit1);

    // List branches
    refs.createBranch("feature", commit2);
    auto branches = refs.listBranches();
    ASSERT_EQ(branches.size(), 2);
    ASSERT_EQ(branches[0], "feature");
    ASSERT_EQ(branches[1], "main");

    // Update HEAD to feature
    refs.updateHeadToBranch("feature");
    ASSERT_EQ(refs.readHead(), "ref: refs/heads/feature");
    ASSERT_EQ(refs.resolveHead().value(), commit2);

    // Detached HEAD
    refs.updateHeadToCommit(commit1);
    ASSERT_EQ(refs.readHead(), commit1);
    ASSERT_EQ(refs.resolveHead().value(), commit1);

    // Branch already exists throws
    ASSERT_THROWS(refs.createBranch("main", commit2), MiniGitException);
}

int main() {
    std::cout << "=== MiniGit RefManager Tests ===\n\n";
    RUN_TEST(test_ref_manager_operations);
    TEST_REPORT();
}
