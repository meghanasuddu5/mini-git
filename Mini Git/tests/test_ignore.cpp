/// Tests for the Ignore class (Phase 8).
///
/// Test plan:
///   1. Exact match
///   2. Directory match (trailing slash)
///   3. Extension match (*.ext)
///   4. .minigit is automatically ignored

#include "test_utils.h"
#include "minigit/index/ignore.h"
#include "minigit/utils/filesystem.h"

#include <fstream>

using namespace minigit;

// ─── Test: Exact match ──────────────────────────────────────────────────────

void test_ignore_exact_match() {
    TempDir tmp;
    fs::path ignorePath = tmp.path() / ".minigitignore";
    utils::writeStringToFile(ignorePath, "secret.txt\n");

    Ignore ignore(ignorePath);

    ASSERT_TRUE(ignore.isIgnored("secret.txt"));
    ASSERT_TRUE(ignore.isIgnored("src/secret.txt"));
    ASSERT_FALSE(ignore.isIgnored("secret.txt.bak"));
    ASSERT_FALSE(ignore.isIgnored("not_secret.txt"));
}

// ─── Test: Directory match ──────────────────────────────────────────────────

void test_ignore_directory_match() {
    TempDir tmp;
    fs::path ignorePath = tmp.path() / ".minigitignore";
    utils::writeStringToFile(ignorePath, "build/\n");

    Ignore ignore(ignorePath);

    ASSERT_TRUE(ignore.isIgnored("build/main.o"));
    ASSERT_TRUE(ignore.isIgnored("build/nested/file.txt"));
    ASSERT_TRUE(ignore.isIgnored("src/build/test.o"));
    ASSERT_FALSE(ignore.isIgnored("build_tools/script.py"));
    ASSERT_FALSE(ignore.isIgnored("builder.cpp"));
}

// ─── Test: Extension match ──────────────────────────────────────────────────

void test_ignore_extension_match() {
    TempDir tmp;
    fs::path ignorePath = tmp.path() / ".minigitignore";
    utils::writeStringToFile(ignorePath, "*.exe\n");

    Ignore ignore(ignorePath);

    ASSERT_TRUE(ignore.isIgnored("minigit.exe"));
    ASSERT_TRUE(ignore.isIgnored("build/test.exe"));
    ASSERT_FALSE(ignore.isIgnored("exe_runner.cpp"));
    ASSERT_FALSE(ignore.isIgnored("test.exe.bak"));
}

// ─── Test: Root match ───────────────────────────────────────────────────────

void test_ignore_root_match() {
    TempDir tmp;
    fs::path ignorePath = tmp.path() / ".minigitignore";
    utils::writeStringToFile(ignorePath, "/root_only.txt\n");

    Ignore ignore(ignorePath);

    ASSERT_TRUE(ignore.isIgnored("root_only.txt"));
    ASSERT_FALSE(ignore.isIgnored("src/root_only.txt"));
}

// ─── Test: minigit directory is automatically ignored ───────────────────────

void test_ignore_minigit_dir() {
    TempDir tmp;
    fs::path ignorePath = tmp.path() / ".minigitignore";
    // Empty ignore file
    utils::writeStringToFile(ignorePath, "");

    Ignore ignore(ignorePath);

    ASSERT_TRUE(ignore.isIgnored(".minigit/config"));
    ASSERT_TRUE(ignore.isIgnored(".minigit/objects/a9/1234"));
    ASSERT_FALSE(ignore.isIgnored("src/main.cpp"));
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== MiniGit Ignore Tests ===\n\n";

    RUN_TEST(test_ignore_exact_match);
    RUN_TEST(test_ignore_directory_match);
    RUN_TEST(test_ignore_extension_match);
    RUN_TEST(test_ignore_root_match);
    RUN_TEST(test_ignore_minigit_dir);

    TEST_REPORT();
}
