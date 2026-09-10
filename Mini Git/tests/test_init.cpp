/// Tests for repository initialization (Phase 1).
///
/// Test plan:
///   1. init creates the full .minigit/ directory structure
///   2. HEAD contains the correct default branch reference
///   3. Re-initializing the same directory fails appropriately
///   4. open() finds a repo from a subdirectory
///   5. open() fails when no repo exists

#include "test_utils.h"
#include "minigit/repository/repository.h"
#include "minigit/utils/filesystem.h"
#include "minigit/utils/error.h"

using namespace minigit;

// ─── Test: init creates directory structure ─────────────────────────────────

void test_init_creates_structure() {
    TempDir tmp;

    Repository repo = Repository::init(tmp.path());

    // Verify all expected directories exist
    ASSERT_TRUE(utils::directoryExists(tmp.path() / ".minigit"));
    ASSERT_TRUE(utils::directoryExists(tmp.path() / ".minigit" / "objects"));
    ASSERT_TRUE(utils::directoryExists(tmp.path() / ".minigit" / "refs"));
    ASSERT_TRUE(utils::directoryExists(tmp.path() / ".minigit" / "refs" / "heads"));

    // Verify all expected files exist
    ASSERT_TRUE(utils::fileExists(tmp.path() / ".minigit" / "HEAD"));
    ASSERT_TRUE(utils::fileExists(tmp.path() / ".minigit" / "index"));
    ASSERT_TRUE(utils::fileExists(tmp.path() / ".minigit" / "config"));
}

// ─── Test: HEAD contains correct default branch reference ───────────────────

void test_init_head_content() {
    TempDir tmp;

    Repository::init(tmp.path());

    std::string headContent = utils::readFileToString(tmp.path() / ".minigit" / "HEAD");
    ASSERT_EQ(headContent, std::string("ref: refs/heads/main\n"));
}

// ─── Test: index file is initially empty ────────────────────────────────────

void test_init_empty_index() {
    TempDir tmp;

    Repository::init(tmp.path());

    std::string indexContent = utils::readFileToString(tmp.path() / ".minigit" / "index");
    ASSERT_EQ(indexContent, std::string(""));
}

// ─── Test: config file contains user section ────────────────────────────────

void test_init_config_content() {
    TempDir tmp;

    Repository::init(tmp.path());

    std::string config = utils::readFileToString(tmp.path() / ".minigit" / "config");
    // Should contain at least the [user] section
    ASSERT_TRUE(config.find("[user]") != std::string::npos);
    ASSERT_TRUE(config.find("name") != std::string::npos);
}

// ─── Test: re-init fails ────────────────────────────────────────────────────

void test_reinit_fails() {
    TempDir tmp;

    Repository::init(tmp.path());

    // Second init on same path should throw
    ASSERT_THROWS(Repository::init(tmp.path()), RepositoryException);
}

// ─── Test: path accessors are consistent ────────────────────────────────────

void test_path_accessors() {
    TempDir tmp;

    Repository repo = Repository::init(tmp.path());

    // All paths should be under the root
    ASSERT_EQ(repo.minigitPath(), repo.rootPath() / ".minigit");
    ASSERT_EQ(repo.objectsPath(), repo.rootPath() / ".minigit" / "objects");
    ASSERT_EQ(repo.refsPath(), repo.rootPath() / ".minigit" / "refs");
    ASSERT_EQ(repo.headsPath(), repo.rootPath() / ".minigit" / "refs" / "heads");
    ASSERT_EQ(repo.headPath(), repo.rootPath() / ".minigit" / "HEAD");
    ASSERT_EQ(repo.indexPath(), repo.rootPath() / ".minigit" / "index");
    ASSERT_EQ(repo.configPath(), repo.rootPath() / ".minigit" / "config");
}

// ─── Test: open() finds repo from subdirectory ─────────────────────────────

void test_open_from_subdirectory() {
    TempDir tmp;

    Repository::init(tmp.path());

    // Create a nested subdirectory
    fs::path subdir = tmp.path() / "src" / "utils";
    fs::create_directories(subdir);

    // open() from subdirectory should find the repo root
    Repository repo = Repository::open(subdir);
    ASSERT_EQ(repo.rootPath(), fs::canonical(tmp.path()));
}

// ─── Test: open() fails when no repo exists ─────────────────────────────────

void test_open_fails_no_repo() {
    TempDir tmp;

    // No init — should fail
    ASSERT_THROWS(Repository::open(tmp.path()), RepositoryException);
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== MiniGit Repository Init Tests ===\n\n";

    RUN_TEST(test_init_creates_structure);
    RUN_TEST(test_init_head_content);
    RUN_TEST(test_init_empty_index);
    RUN_TEST(test_init_config_content);
    RUN_TEST(test_reinit_fails);
    RUN_TEST(test_path_accessors);
    RUN_TEST(test_open_from_subdirectory);
    RUN_TEST(test_open_fails_no_repo);

    TEST_REPORT();
}
