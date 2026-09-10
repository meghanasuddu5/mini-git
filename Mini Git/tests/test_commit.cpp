/// Tests for the Commit object (Phase 5).
///
/// Test plan:
///   1. Commit serialization/deserialization roundtrip (initial commit)
///   2. Commit serialization/deserialization roundtrip (with parent)
///   3. Missing fields throw exceptions
///   4. Different timestamps produce different hashes

#include "test_utils.h"
#include "minigit/object/commit.h"
#include "minigit/object/hash.h"
#include "minigit/utils/error.h"

using namespace minigit;

// ─── Test: serialization roundtrip (initial commit) ─────────────────────────

void test_commit_roundtrip_initial() {
    std::string treeHash = std::string(64, 'a');
    std::string author = "Test User";
    std::string message = "Initial commit";
    uint64_t timestamp = 1234567890;

    Commit commit(treeHash, std::nullopt, author, timestamp, message);
    std::string serialized = commit.serialize();

    Commit restored = Commit::deserialize(serialized);

    ASSERT_EQ(restored.treeHash(), treeHash);
    ASSERT_FALSE(restored.parentHash().has_value());
    ASSERT_EQ(restored.author(), author);
    ASSERT_EQ(restored.timestamp(), timestamp);
    ASSERT_EQ(restored.message(), message);
}

// ─── Test: serialization roundtrip (with parent) ────────────────────────────

void test_commit_roundtrip_with_parent() {
    std::string treeHash = std::string(64, 'b');
    std::string parentHash = std::string(64, 'c');
    std::string author = "Another User";
    std::string message = "Fix a bug\n\nDetailed description here.";
    uint64_t timestamp = 9876543210;

    Commit commit(treeHash, parentHash, author, timestamp, message);
    std::string serialized = commit.serialize();

    Commit restored = Commit::deserialize(serialized);

    ASSERT_EQ(restored.treeHash(), treeHash);
    ASSERT_TRUE(restored.parentHash().has_value());
    ASSERT_EQ(restored.parentHash().value(), parentHash);
    ASSERT_EQ(restored.author(), author);
    ASSERT_EQ(restored.timestamp(), timestamp);
    ASSERT_EQ(restored.message(), message);
}

// ─── Test: missing fields throw ─────────────────────────────────────────────

void test_malformed_commit_throws() {
    // Valid commit format
    std::string validBody = "tree " + std::string(64, 'a') + "\n"
                          + "author Tester 1234\n"
                          + "\n"
                          + "msg\n";
    std::string valid = "commit " + std::to_string(validBody.size()) + '\0' + validBody;

    try {
        Commit::deserialize(valid);
    } catch (...) {
        ASSERT_TRUE(false); // Should not throw
    }

    // Missing tree
    std::string noTreeBody = "author Tester 1234\n\nmsg\n";
    std::string noTree = "commit " + std::to_string(noTreeBody.size()) + '\0' + noTreeBody;
    ASSERT_THROWS(Commit::deserialize(noTree), MiniGitException);

    // Missing message
    std::string noMsgBody = "tree " + std::string(64, 'a') + "\n"
                          + "author Tester 1234\n\n";
    std::string noMsg = "commit " + std::to_string(noMsgBody.size()) + '\0' + noMsgBody;
    ASSERT_THROWS(Commit::deserialize(noMsg), MiniGitException);
}

// ─── Test: different timestamps produce different hashes ────────────────────

void test_different_timestamps_hashes() {
    std::string treeHash = std::string(64, 't');
    std::string author = "User";
    std::string message = "msg";

    Commit c1(treeHash, std::nullopt, author, 1000, message);
    Commit c2(treeHash, std::nullopt, author, 1001, message);

    ASSERT_NE(SHA256::hashString(c1.serialize()),
              SHA256::hashString(c2.serialize()));
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== MiniGit Commit Tests ===\n\n";

    RUN_TEST(test_commit_roundtrip_initial);
    RUN_TEST(test_commit_roundtrip_with_parent);
    RUN_TEST(test_malformed_commit_throws);
    RUN_TEST(test_different_timestamps_hashes);

    TEST_REPORT();
}
