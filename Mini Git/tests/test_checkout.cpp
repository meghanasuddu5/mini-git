/// Tests for Checkout (Phase 9).

#include "test_utils.h"
#include "minigit/repository/checkout.h"
#include "minigit/repository/repository.h"
#include "minigit/index/index.h"
#include "minigit/object/object_store.h"
#include "minigit/object/tree.h"
#include "minigit/object/commit.h"
#include "minigit/utils/filesystem.h"

using namespace minigit;

void test_checkout_branch() {
    TempDir tmp;
    
    // Init repo manually since we didn't expose CLI Init function directly
    fs::path repoPath = tmp.path();
    fs::path minigitPath = repoPath / ".minigit";
    fs::create_directories(minigitPath / "objects");
    fs::create_directories(minigitPath / "refs" / "heads");
    utils::writeStringToFile(minigitPath / "HEAD", "ref: refs/heads/main\n");

    ObjectStore store(minigitPath / "objects");
    Index index(minigitPath / "index", minigitPath / "objects", repoPath);

    // ─── Setup Commit 1 ─────────────────────────────────────────────────────
    
    utils::writeStringToFile(repoPath / "file1.txt", "v1");
    index.addFile(repoPath / "file1.txt");
    
    TreeBuilder tb1;
    for (const auto& [relPath, entry] : index.entries()) {
        tb1.addEntry(relPath, entry.blobHash);
    }
    std::string treeHash1 = tb1.build(store);
    
    Commit c1(treeHash1, std::nullopt, "Tester", 1000, "Commit 1");
    std::string commitHash1 = store.writeRaw(c1.serialize());

    // ─── Setup Commit 2 ─────────────────────────────────────────────────────

    utils::writeStringToFile(repoPath / "file1.txt", "v2");
    utils::writeStringToFile(repoPath / "file2.txt", "hello");
    index.addFile(repoPath / "file1.txt");
    index.addFile(repoPath / "file2.txt");

    TreeBuilder tb2;
    for (const auto& [relPath, entry] : index.entries()) {
        tb2.addEntry(relPath, entry.blobHash);
    }
    std::string treeHash2 = tb2.build(store);
    
    Commit c2(treeHash2, commitHash1, "Tester", 2000, "Commit 2");
    std::string commitHash2 = store.writeRaw(c2.serialize());

    // ─── Perform Checkout ───────────────────────────────────────────────────

    Checkout checkout(repoPath, minigitPath / "objects");
    
    // Currently on commit 2 state. Let's checkout commit 1.
    checkout.checkoutCommit(commitHash1, index);

    // file2.txt should be deleted
    ASSERT_FALSE(utils::fileExists(repoPath / "file2.txt"));
    
    // file1.txt should have "v1"
    ASSERT_EQ(utils::readFileToString(repoPath / "file1.txt"), "v1");

    // Index should only have file1.txt
    ASSERT_TRUE(index.getBlobHash("file1.txt").has_value());
    ASSERT_FALSE(index.getBlobHash("file2.txt").has_value());
    
    // Now checkout commit 2 again
    checkout.checkoutCommit(commitHash2, index);

    // file2.txt should exist
    ASSERT_TRUE(utils::fileExists(repoPath / "file2.txt"));
    ASSERT_EQ(utils::readFileToString(repoPath / "file2.txt"), "hello");
    
    // file1.txt should have "v2"
    ASSERT_EQ(utils::readFileToString(repoPath / "file1.txt"), "v2");

    // Index should have both
    ASSERT_TRUE(index.getBlobHash("file1.txt").has_value());
    ASSERT_TRUE(index.getBlobHash("file2.txt").has_value());
}

int main() {
    std::cout << "=== MiniGit Checkout Tests ===\n\n";
    RUN_TEST(test_checkout_branch);
    TEST_REPORT();
}
