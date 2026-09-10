/// Tests for CLI argument parsing (Phase 1).
///
/// Test plan:
///   1. No arguments → run() returns 1 (usage printed)
///   2. --help → run() returns 0
///   3. --version → run() returns 0
///   4. Unknown command → run() returns 1
///   5. Known future command → run() returns 1 (not yet implemented)
///   6. init command → run() returns 0 (repo created)

#include "test_utils.h"
#include "minigit/cli/cli.h"

#include <filesystem>

using namespace minigit;

// Helper: construct a CLI from a vector of strings (simulates argv).
// We need this because CLI takes char*[], not std::string.
class CLIArgs {
public:
    CLIArgs(std::initializer_list<std::string> argList) {
        for (const auto& arg : argList) {
            storage_.push_back(arg);
        }
        for (auto& s : storage_) {
            ptrs_.push_back(s.data());
        }
    }

    int argc() const { return static_cast<int>(ptrs_.size()); }
    char** argv() { return ptrs_.data(); }

private:
    std::vector<std::string> storage_;
    std::vector<char*> ptrs_;
};

// ─── Test: no arguments ─────────────────────────────────────────────────────

void test_no_args_returns_error() {
    CLIArgs args{"minigit"};
    CLI cli(args.argc(), args.argv());
    ASSERT_EQ(cli.run(), 1);
}

// ─── Test: --help ───────────────────────────────────────────────────────────

void test_help_returns_success() {
    CLIArgs args{"minigit", "--help"};
    CLI cli(args.argc(), args.argv());
    ASSERT_EQ(cli.run(), 0);
}

// ─── Test: --version ────────────────────────────────────────────────────────

void test_version_returns_success() {
    CLIArgs args{"minigit", "--version"};
    CLI cli(args.argc(), args.argv());
    ASSERT_EQ(cli.run(), 0);
}

// ─── Test: unknown command ──────────────────────────────────────────────────

void test_unknown_command_returns_error() {
    CLIArgs args{"minigit", "frobnicate"};
    CLI cli(args.argc(), args.argv());
    ASSERT_EQ(cli.run(), 1);
}

// ─── Test: not-yet-implemented command ──────────────────────────────────────

void test_future_command_returns_error() {
    CLIArgs args{"minigit", "add"};
    CLI cli(args.argc(), args.argv());
    ASSERT_EQ(cli.run(), 1);
}

// ─── Test: init succeeds ────────────────────────────────────────────────────

void test_init_command_succeeds() {
    TempDir tmp;

    CLIArgs args{"minigit", "init", tmp.path().string() + "/myrepo"};
    CLI cli(args.argc(), args.argv());

    int result = cli.run();
    ASSERT_EQ(result, 0);

    // Verify .minigit was created inside myrepo
    ASSERT_TRUE(fs::exists(tmp.path() / "myrepo" / ".minigit"));
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== MiniGit CLI Tests ===\n\n";

    RUN_TEST(test_no_args_returns_error);
    RUN_TEST(test_help_returns_success);
    RUN_TEST(test_version_returns_success);
    RUN_TEST(test_unknown_command_returns_error);
    RUN_TEST(test_future_command_returns_error);
    RUN_TEST(test_init_command_succeeds);

    TEST_REPORT();
}
