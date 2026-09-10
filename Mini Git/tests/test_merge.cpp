/// Tests for 3-Way Merge & MergeEngine (Phase 11).

#include "test_utils.h"
#include "minigit/repository/merge.h"
#include "minigit/repository/graph_analyzer.h"
#include "minigit/repository/ref_manager.h"
#include "minigit/repository/checkout.h"
#include "minigit/index/index.h"
#include "minigit/object/object_store.h"
#include "minigit/object/tree.h"
#include "minigit/object/commit.h"
#include "minigit/utils/filesystem.h"

using namespace minigit;

void test_merge_fast_forward() {
    TempDir tmp;
    fs::path repoPath = tmp.path();
    fs::path minigitPath = repoPath / ".minigit";
    fs::create_directories(minigitPath / "objects");
    fs::create_directories(minigitPath / "refs" / "heads");
    utils::writeStringToFile(minigitPath / "HEAD", "ref: refs/heads/main\n");

    ObjectStore store(minigitPath / "objects");
    Index index(minigitPath / "index", minigitPath / "objects", repoPath);
    RefManager refs(minigitPath);

    // Commit 1 (main)
    utils::writeStringToFile(repoPath / "file1.txt", "v1");
    index.addFile(repoPath / "file1.txt");
    
    TreeBuilder tb1;
    for (const auto& [relPath, entry] : index.entries()) {
        tb1.addEntry(relPath, entry.blobHash);
    }
    std::string treeHash1 = tb1.build(store);
    
    Commit c1(treeHash1, std::nullopt, "Tester", 1000, "Commit 1");
    std::string commitHash1 = store.writeRaw(c1.serialize());
    refs.createBranch("main", commitHash1);

    // Commit 2 (feature)
    utils::writeStringToFile(repoPath / "file1.txt", "v2");
    index.addFile(repoPath / "file1.txt");

    TreeBuilder tb2;
    for (const auto& [relPath, entry] : index.entries()) {
        tb2.addEntry(relPath, entry.blobHash);
    }
    std::string treeHash2 = tb2.build(store);
    
    Commit c2(treeHash2, commitHash1, "Tester", 2000, "Commit 2");
    std::string commitHash2 = store.writeRaw(c2.serialize());
    refs.createBranch("feature", commitHash2);

    // Check fast-forward reachability
    ASSERT_TRUE(GraphAnalyzer::isAncestor(commitHash1, commitHash2, store));

    Checkout checkout(repoPath, minigitPath / "objects");
    checkout.checkoutCommit(commitHash2, index);
    refs.updateBranch("main", commitHash2);

    ASSERT_EQ(refs.resolveBranch("main").value(), commitHash2);
    ASSERT_EQ(utils::readFileToString(repoPath / "file1.txt"), "v2");
}

void test_merge_3way_independent_files() {
    TempDir tmp;
    fs::path repoPath = tmp.path();
    fs::path objDir = repoPath / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    // Base tree: file1.txt
    Blob b1("file1 base content\n");
    std::string h1 = store.writeBlob(b1);
    Tree baseTree({{"100644", ObjectType::Blob, h1, "file1.txt"}});
    std::string baseTreeHash = store.writeRaw(baseTree.serialize());

    // Ours tree: file1.txt + file_ours.txt
    Blob bOurs("file_ours content\n");
    std::string hOurs = store.writeBlob(bOurs);
    Tree oursTree({
        {"100644", ObjectType::Blob, h1, "file1.txt"},
        {"100644", ObjectType::Blob, hOurs, "file_ours.txt"}
    });
    std::string oursTreeHash = store.writeRaw(oursTree.serialize());

    // Theirs tree: file1.txt + file_theirs.txt
    Blob bTheirs("file_theirs content\n");
    std::string hTheirs = store.writeBlob(bTheirs);
    Tree theirsTree({
        {"100644", ObjectType::Blob, h1, "file1.txt"},
        {"100644", ObjectType::Blob, hTheirs, "file_theirs.txt"}
    });
    std::string theirsTreeHash = store.writeRaw(theirsTree.serialize());

    // Merge!
    MergeResult res = MergeEngine::mergeTrees(baseTreeHash, oursTreeHash, theirsTreeHash, store, "main", "feature");

    ASSERT_FALSE(res.hasConflicts);
    ASSERT_EQ(res.fileBlobs.size(), static_cast<size_t>(3));
    ASSERT_TRUE(res.fileBlobs.count("file1.txt"));
    ASSERT_TRUE(res.fileBlobs.count("file_ours.txt"));
    ASSERT_TRUE(res.fileBlobs.count("file_theirs.txt"));
}

void test_merge_3way_conflicts() {
    TempDir tmp;
    fs::path repoPath = tmp.path();
    fs::path objDir = repoPath / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    // Base tree: common.txt = "line1\nline2\nline3\n"
    Blob bBase("line1\nline2\nline3\n");
    std::string hBase = store.writeBlob(bBase);
    Tree baseTree({{"100644", ObjectType::Blob, hBase, "common.txt"}});
    std::string baseTreeHash = store.writeRaw(baseTree.serialize());

    // Ours tree: common.txt = "line1\nOURS EDIT\nline3\n"
    Blob bOurs("line1\nOURS EDIT\nline3\n");
    std::string hOurs = store.writeBlob(bOurs);
    Tree oursTree({{"100644", ObjectType::Blob, hOurs, "common.txt"}});
    std::string oursTreeHash = store.writeRaw(oursTree.serialize());

    // Theirs tree: common.txt = "line1\nTHEIRS EDIT\nline3\n"
    Blob bTheirs("line1\nTHEIRS EDIT\nline3\n");
    std::string hTheirs = store.writeBlob(bTheirs);
    Tree theirsTree({{"100644", ObjectType::Blob, hTheirs, "common.txt"}});
    std::string theirsTreeHash = store.writeRaw(theirsTree.serialize());

    // Merge!
    MergeResult res = MergeEngine::mergeTrees(baseTreeHash, oursTreeHash, theirsTreeHash, store, "main", "feature");

    ASSERT_TRUE(res.hasConflicts);
    ASSERT_EQ(res.conflictedFiles.size(), static_cast<size_t>(1));
    ASSERT_EQ(res.conflictedFiles[0], "common.txt");

    // Check conflict markers in content
    std::string content = res.fileContents["common.txt"];
    ASSERT_TRUE(content.find("<<<<<<< main") != std::string::npos);
    ASSERT_TRUE(content.find("OURS EDIT") != std::string::npos);
    ASSERT_TRUE(content.find("=======") != std::string::npos);
    ASSERT_TRUE(content.find("THEIRS EDIT") != std::string::npos);
    ASSERT_TRUE(content.find(">>>>>>> feature") != std::string::npos);
}

int main() {
    std::cout << "=== MiniGit Merge Tests ===\n\n";
    RUN_TEST(test_merge_fast_forward);
    RUN_TEST(test_merge_3way_independent_files);
    RUN_TEST(test_merge_3way_conflicts);
    TEST_REPORT();
}
