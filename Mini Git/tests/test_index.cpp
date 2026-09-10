/// Tests for the Index / Staging Area (Phase 3).
///
/// Test plan:
///   1. Add a single file — creates blob and index entry
///   2. Add multiple files
///   3. Index persistence (save → load roundtrip)
///   4. Modified file detected by status
///   5. Deleted file detected by status
///   6. Untracked file detected by status
///   7. Staged (unchanged) file detected by status
///   8. addAll stages everything
///   9. Remove file from index
///  10. Empty index

#include "test_utils.h"
#include "minigit/index/index.h"
#include "minigit/repository/repository.h"
#include "minigit/utils/filesystem.h"
#include "minigit/utils/error.h"

#include <fstream>

using namespace minigit;

/// Helper: initialize a repo in a temp dir and return the repo object.
static Repository setupRepo(const fs::path& dir) {
    return Repository::init(dir);
}

/// Helper: create a file with given content.
static void createFile(const fs::path& path, const std::string& content) {
    utils::writeStringToFile(path, content);
}

// ─── Test: add a single file ────────────────────────────────────────────────

void test_add_single_file() {
    TempDir tmp;
    auto repo = setupRepo(tmp.path());

    // Create a file in the working tree
    createFile(tmp.path() / "hello.txt", "Hello, World!\n");

    // Stage it
    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    index.addFile(tmp.path() / "hello.txt");

    // Verify it's in the index
    ASSERT_TRUE(index.contains("hello.txt"));
    ASSERT_EQ(index.size(), static_cast<size_t>(1));

    // Verify the blob was written to the object store
    auto hash = index.getBlobHash("hello.txt");
    ASSERT_TRUE(hash.has_value());
    ASSERT_EQ(hash.value().size(), static_cast<size_t>(64)); // SHA-256 hex
}

// ─── Test: add multiple files ───────────────────────────────────────────────

void test_add_multiple_files() {
    TempDir tmp;
    auto repo = setupRepo(tmp.path());

    createFile(tmp.path() / "file1.txt", "content 1");
    createFile(tmp.path() / "file2.txt", "content 2");
    createFile(tmp.path() / "file3.txt", "content 3");

    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    index.addFile(tmp.path() / "file1.txt");
    index.addFile(tmp.path() / "file2.txt");
    index.addFile(tmp.path() / "file3.txt");

    ASSERT_EQ(index.size(), static_cast<size_t>(3));
    ASSERT_TRUE(index.contains("file1.txt"));
    ASSERT_TRUE(index.contains("file2.txt"));
    ASSERT_TRUE(index.contains("file3.txt"));
}

// ─── Test: index persistence ────────────────────────────────────────────────

void test_index_persistence() {
    TempDir tmp;
    auto repo = setupRepo(tmp.path());

    createFile(tmp.path() / "persist.txt", "persistent content");

    // Add file with first Index instance
    {
        Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
        index.addFile(tmp.path() / "persist.txt");
    }

    // Create a NEW Index instance — should load from disk
    Index index2(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    ASSERT_TRUE(index2.contains("persist.txt"));
    ASSERT_EQ(index2.size(), static_cast<size_t>(1));
}

// ─── Test: status detects modified file ─────────────────────────────────────

void test_status_modified() {
    TempDir tmp;
    auto repo = setupRepo(tmp.path());

    createFile(tmp.path() / "changing.txt", "version 1");

    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    index.addFile(tmp.path() / "changing.txt");

    // Modify the file AFTER staging
    createFile(tmp.path() / "changing.txt", "version 2");

    auto status = index.computeStatus();

    // Find the entry for changing.txt
    bool foundModified = false;
    for (const auto& entry : status) {
        if (entry.path == "changing.txt") {
            ASSERT_EQ(static_cast<int>(entry.status),
                      static_cast<int>(FileStatus::Modified));
            foundModified = true;
        }
    }
    ASSERT_TRUE(foundModified);
}

// ─── Test: status detects deleted file ──────────────────────────────────────

void test_status_deleted() {
    TempDir tmp;
    auto repo = setupRepo(tmp.path());

    createFile(tmp.path() / "doomed.txt", "will be deleted");

    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    index.addFile(tmp.path() / "doomed.txt");

    // Delete the file from the working tree
    fs::remove(tmp.path() / "doomed.txt");

    auto status = index.computeStatus();

    bool foundDeleted = false;
    for (const auto& entry : status) {
        if (entry.path == "doomed.txt") {
            ASSERT_EQ(static_cast<int>(entry.status),
                      static_cast<int>(FileStatus::Deleted));
            foundDeleted = true;
        }
    }
    ASSERT_TRUE(foundDeleted);
}

// ─── Test: status detects untracked file ────────────────────────────────────

void test_status_untracked() {
    TempDir tmp;
    auto repo = setupRepo(tmp.path());

    // Create a file but don't add it
    createFile(tmp.path() / "new_file.txt", "untracked content");

    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    auto status = index.computeStatus();

    bool foundUntracked = false;
    for (const auto& entry : status) {
        if (entry.path == "new_file.txt") {
            ASSERT_EQ(static_cast<int>(entry.status),
                      static_cast<int>(FileStatus::Untracked));
            foundUntracked = true;
        }
    }
    ASSERT_TRUE(foundUntracked);
}

// ─── Test: status shows staged (unchanged) file ────────────────────────────

void test_status_staged() {
    TempDir tmp;
    auto repo = setupRepo(tmp.path());

    createFile(tmp.path() / "stable.txt", "unchanged content");

    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    index.addFile(tmp.path() / "stable.txt");

    // Don't modify the file — it should be "Staged"
    auto status = index.computeStatus();

    bool foundStaged = false;
    for (const auto& entry : status) {
        if (entry.path == "stable.txt") {
            ASSERT_EQ(static_cast<int>(entry.status),
                      static_cast<int>(FileStatus::Staged));
            foundStaged = true;
        }
    }
    ASSERT_TRUE(foundStaged);
}

// ─── Test: addAll stages all files ──────────────────────────────────────────

void test_add_all() {
    TempDir tmp;
    auto repo = setupRepo(tmp.path());

    createFile(tmp.path() / "a.txt", "aaa");
    createFile(tmp.path() / "b.txt", "bbb");
    fs::create_directories(tmp.path() / "subdir");
    createFile(tmp.path() / "subdir" / "c.txt", "ccc");

    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    index.addAll();

    ASSERT_EQ(index.size(), static_cast<size_t>(3));
    ASSERT_TRUE(index.contains("a.txt"));
    ASSERT_TRUE(index.contains("b.txt"));
    ASSERT_TRUE(index.contains("subdir/c.txt"));
}

// ─── Test: remove file from index ───────────────────────────────────────────

void test_remove_file() {
    TempDir tmp;
    auto repo = setupRepo(tmp.path());

    createFile(tmp.path() / "removeme.txt", "bye");

    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    index.addFile(tmp.path() / "removeme.txt");
    ASSERT_TRUE(index.contains("removeme.txt"));

    index.removeFile("removeme.txt");
    ASSERT_FALSE(index.contains("removeme.txt"));
    ASSERT_EQ(index.size(), static_cast<size_t>(0));
}

// ─── Test: adding same file twice updates hash ──────────────────────────────

void test_readd_updates_hash() {
    TempDir tmp;
    auto repo = setupRepo(tmp.path());

    createFile(tmp.path() / "evolving.txt", "version 1");

    Index index(repo.indexPath(), repo.objectsPath(), repo.rootPath());
    index.addFile(tmp.path() / "evolving.txt");
    auto hash1 = index.getBlobHash("evolving.txt");

    // Modify and re-add
    createFile(tmp.path() / "evolving.txt", "version 2");
    index.addFile(tmp.path() / "evolving.txt");
    auto hash2 = index.getBlobHash("evolving.txt");

    // Hash should have changed
    ASSERT_NE(hash1.value(), hash2.value());

    // Should still be only one entry
    ASSERT_EQ(index.size(), static_cast<size_t>(1));
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== MiniGit Index Tests ===\n\n";

    RUN_TEST(test_add_single_file);
    RUN_TEST(test_add_multiple_files);
    RUN_TEST(test_index_persistence);
    RUN_TEST(test_status_modified);
    RUN_TEST(test_status_deleted);
    RUN_TEST(test_status_untracked);
    RUN_TEST(test_status_staged);
    RUN_TEST(test_add_all);
    RUN_TEST(test_remove_file);
    RUN_TEST(test_readd_updates_hash);

    TEST_REPORT();
}
