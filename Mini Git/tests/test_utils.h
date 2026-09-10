#pragma once

/// Lightweight test framework for MiniGit.
///
/// Provides:
///   - ASSERT_TRUE(cond)        — fails if condition is false
///   - ASSERT_FALSE(cond)       — fails if condition is true
///   - ASSERT_EQ(a, b)          — fails if a != b
///   - ASSERT_NE(a, b)          — fails if a == b
///   - ASSERT_THROWS(expr, T)   — fails if expr doesn't throw type T
///   - RUN_TEST(func)           — runs a test function and tracks pass/fail
///   - TEST_REPORT()            — prints summary and returns exit code
///   - TempDir                  — RAII temporary directory for test isolation

#include <filesystem>
#include <iostream>
#include <string>
#include <functional>

namespace fs = std::filesystem;

// ─── Assertion Macros ───────────────────────────────────────────────────────
// These macros print a clear error message with file/line on failure,
// then return from the test function (so subsequent assertions are skipped).

#define ASSERT_TRUE(cond)                                                     \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__            \
                      << ": ASSERT_TRUE(" #cond ") failed\n";                 \
            throw TestFailure{};                                              \
        }                                                                     \
    } while (false)

#define ASSERT_FALSE(cond)                                                    \
    do {                                                                      \
        if ((cond)) {                                                         \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__            \
                      << ": ASSERT_FALSE(" #cond ") failed\n";               \
            throw TestFailure{};                                              \
        }                                                                     \
    } while (false)

#define ASSERT_EQ(a, b)                                                       \
    do {                                                                      \
        if ((a) != (b)) {                                                     \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__            \
                      << ": ASSERT_EQ(" #a ", " #b ") failed\n"              \
                      << "    left:  " << (a) << "\n"                         \
                      << "    right: " << (b) << "\n";                        \
            throw TestFailure{};                                              \
        }                                                                     \
    } while (false)

#define ASSERT_NE(a, b)                                                       \
    do {                                                                      \
        if ((a) == (b)) {                                                     \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__            \
                      << ": ASSERT_NE(" #a ", " #b ") failed\n"              \
                      << "    both: " << (a) << "\n";                         \
            throw TestFailure{};                                              \
        }                                                                     \
    } while (false)

#define ASSERT_THROWS(expr, exType)                                           \
    do {                                                                      \
        bool caught = false;                                                  \
        try {                                                                 \
            expr;                                                             \
        } catch (const exType&) {                                             \
            caught = true;                                                    \
        } catch (...) {                                                       \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__            \
                      << ": ASSERT_THROWS(" #expr ", " #exType ")"            \
                      << " caught wrong exception type\n";                    \
            throw TestFailure{};                                              \
        }                                                                     \
        if (!caught) {                                                        \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__            \
                      << ": ASSERT_THROWS(" #expr ", " #exType ")"            \
                      << " did not throw\n";                                  \
            throw TestFailure{};                                              \
        }                                                                     \
    } while (false)

// ─── Test Runner ────────────────────────────────────────────────────────────

/// Sentinel exception used by assertion macros to abort a test.
struct TestFailure {};

/// Global counters (scoped to each test executable).
inline int g_testsPassed = 0;
inline int g_testsFailed = 0;

/// Run a single test function, catching failures and unexpected exceptions.
#define RUN_TEST(func)                                                        \
    do {                                                                      \
        std::cout << "[ RUN      ] " << #func << "\n";                        \
        try {                                                                 \
            func();                                                           \
            std::cout << "[       OK ] " << #func << "\n";                    \
            ++g_testsPassed;                                                  \
        } catch (const TestFailure&) {                                        \
            std::cout << "[  FAILED  ] " << #func << "\n";                    \
            ++g_testsFailed;                                                  \
        } catch (const std::exception& e) {                                   \
            std::cerr << "  FAIL: unexpected exception: " << e.what() << "\n";\
            std::cout << "[  FAILED  ] " << #func << "\n";                    \
            ++g_testsFailed;                                                  \
        }                                                                     \
    } while (false)

/// Print the test summary and return the appropriate exit code.
#define TEST_REPORT()                                                         \
    do {                                                                      \
        std::cout << "\n========================================\n";          \
        std::cout << g_testsPassed << " passed, "                             \
                  << g_testsFailed << " failed\n";                            \
        return (g_testsFailed == 0) ? 0 : 1;                                  \
    } while (false)

// ─── TempDir RAII Fixture ───────────────────────────────────────────────────

/// Creates a unique temporary directory on construction, removes it
/// (and all contents) on destruction. This ensures tests don't
/// pollute each other or the real filesystem.
///
/// Usage:
///   TempDir tmp;
///   // tmp.path() is an empty, unique directory
///   // automatically cleaned up when tmp goes out of scope
class TempDir {
public:
    TempDir() {
        // Create a unique directory name based on a counter
        static int counter = 0;
        path_ = fs::temp_directory_path() / ("minigit_test_" + std::to_string(counter++));

        // Remove if it somehow exists from a previous crashed test
        if (fs::exists(path_)) {
            fs::remove_all(path_);
        }

        fs::create_directories(path_);
    }

    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
        // Silently ignore cleanup errors in tests
    }

    // Non-copyable, non-movable (RAII resource)
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    [[nodiscard]] const fs::path& path() const { return path_; }

private:
    fs::path path_;
};
