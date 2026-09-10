/// Tests for the SHA-256 hash engine (Phase 2).
///
/// We verify our implementation against known test vectors from
/// the NIST FIPS 180-4 specification and other well-known inputs.

#include "test_utils.h"
#include "minigit/object/hash.h"

using namespace minigit;

// ─── Test: empty string ─────────────────────────────────────────────────────
// Known SHA-256 of "" = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

void test_hash_empty_string() {
    std::string hash = SHA256::hashString("");
    ASSERT_EQ(hash, std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
}

// ─── Test: "abc" ────────────────────────────────────────────────────────────
// NIST test vector: SHA-256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad

void test_hash_abc() {
    std::string hash = SHA256::hashString("abc");
    ASSERT_EQ(hash, std::string("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
}

// ─── Test: longer string ────────────────────────────────────────────────────
// SHA-256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")
// = 248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1

void test_hash_long_string() {
    std::string input = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    std::string hash = SHA256::hashString(input);
    ASSERT_EQ(hash, std::string("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
}

// ─── Test: determinism ──────────────────────────────────────────────────────
// Same input must always produce the same hash.

void test_hash_determinism() {
    std::string input = "Hello, MiniGit!";
    std::string hash1 = SHA256::hashString(input);
    std::string hash2 = SHA256::hashString(input);
    ASSERT_EQ(hash1, hash2);
}

// ─── Test: different inputs produce different hashes ────────────────────────

void test_hash_different_inputs() {
    std::string hash1 = SHA256::hashString("file version 1");
    std::string hash2 = SHA256::hashString("file version 2");
    ASSERT_NE(hash1, hash2);
}

// ─── Test: incremental hashing ──────────────────────────────────────────────
// Feeding data in chunks must produce the same result as all at once.

void test_hash_incremental() {
    std::string full = "Hello, World! This is a test of incremental hashing.";

    // One-shot
    std::string hashFull = SHA256::hashString(full);

    // Incremental (split at an arbitrary point)
    SHA256 hasher;
    hasher.update("Hello, World! ");
    hasher.update("This is a test ");
    hasher.update("of incremental hashing.");
    std::string hashIncremental = hasher.hexDigest();

    ASSERT_EQ(hashFull, hashIncremental);
}

// ─── Test: hash length ──────────────────────────────────────────────────────

void test_hash_length() {
    std::string hash = SHA256::hashString("test");
    ASSERT_EQ(hash.size(), static_cast<size_t>(64));  // 64 hex chars = 256 bits
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== MiniGit SHA-256 Hash Tests ===\n\n";

    RUN_TEST(test_hash_empty_string);
    RUN_TEST(test_hash_abc);
    RUN_TEST(test_hash_long_string);
    RUN_TEST(test_hash_determinism);
    RUN_TEST(test_hash_different_inputs);
    RUN_TEST(test_hash_incremental);
    RUN_TEST(test_hash_length);

    TEST_REPORT();
}
