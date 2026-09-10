/// Tests for the ObjectStore and Blob (Phase 2).
///
/// Test plan:
///   1. Blob serialization roundtrip
///   2. Same content produces same object ID (deduplication)
///   3. Different content produces different IDs
///   4. Object can be written and read back
///   5. Reading non-existent object returns nullopt
///   6. Corrupted object is detected
///   7. Multiple blobs can coexist
///   8. Malformed blob deserialization fails cleanly

#include "test_utils.h"
#include "minigit/object/object.h"
#include "minigit/object/object_store.h"
#include "minigit/object/hash.h"
#include "minigit/utils/error.h"
#include "minigit/utils/filesystem.h"

#include <fstream>

using namespace minigit;

// ─── Test: Blob serialize/deserialize roundtrip ─────────────────────────────

void test_blob_roundtrip() {
    std::string content = "Hello, World!\nThis is a test file.\n";
    Blob original(content);

    std::string serialized = original.serialize();
    Blob restored = Blob::deserialize(serialized);

    ASSERT_EQ(original.content(), restored.content());
}

// ─── Test: Blob serialization format ────────────────────────────────────────

void test_blob_serialization_format() {
    Blob blob("abc");
    std::string serialized = blob.serialize();

    // Should start with "blob 3\0"
    ASSERT_EQ(serialized.substr(0, 6), std::string("blob 3"));
    ASSERT_EQ(serialized[6], '\0');
    ASSERT_EQ(serialized.substr(7), std::string("abc"));
}

// ─── Test: same content → same hash ─────────────────────────────────────────

void test_same_content_same_id() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    Blob blob1("identical content");
    Blob blob2("identical content");

    std::string id1 = store.writeBlob(blob1);
    std::string id2 = store.writeBlob(blob2);

    ASSERT_EQ(id1, id2);
}

// ─── Test: different content → different hash ───────────────────────────────

void test_different_content_different_id() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    Blob blob1("version 1");
    Blob blob2("version 2");

    std::string id1 = store.writeBlob(blob1);
    std::string id2 = store.writeBlob(blob2);

    ASSERT_NE(id1, id2);
}

// ─── Test: write then read ──────────────────────────────────────────────────

void test_write_read_blob() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    std::string content = "file contents\nwith multiple lines\n";
    Blob original(content);
    std::string id = store.writeBlob(original);

    // Read it back
    Blob restored = store.readBlob(id);
    ASSERT_EQ(restored.content(), content);
}

// ─── Test: read non-existent object ─────────────────────────────────────────

void test_read_nonexistent() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    auto result = store.readRaw("0000000000000000000000000000000000000000000000000000000000000000");
    ASSERT_FALSE(result.has_value());
}

// ─── Test: exists() ─────────────────────────────────────────────────────────

void test_exists() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    Blob blob("test content");
    std::string id = store.writeBlob(blob);

    ASSERT_TRUE(store.exists(id));
    ASSERT_FALSE(store.exists("0000000000000000000000000000000000000000000000000000000000000000"));
}

// ─── Test: corrupted object detection ───────────────────────────────────────

void test_corrupted_object_detected() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    // Write a valid blob
    Blob blob("original content");
    std::string id = store.writeBlob(blob);

    // Corrupt the file on disk by overwriting it with different data
    fs::path objPath = objDir / id.substr(0, 2) / id.substr(2);
    {
        std::ofstream file(objPath, std::ios::trunc);
        file << "blob 16\0corrupted data!!";
    }

    // Reading should detect the corruption (hash mismatch)
    ASSERT_THROWS(store.readRaw(id), MiniGitException);
}

// ─── Test: multiple blobs ───────────────────────────────────────────────────

void test_multiple_blobs() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    std::string id1 = store.writeBlob(Blob("file1.txt contents"));
    std::string id2 = store.writeBlob(Blob("file2.txt contents"));
    std::string id3 = store.writeBlob(Blob("file3.txt contents"));

    ASSERT_EQ(store.readBlob(id1).content(), std::string("file1.txt contents"));
    ASSERT_EQ(store.readBlob(id2).content(), std::string("file2.txt contents"));
    ASSERT_EQ(store.readBlob(id3).content(), std::string("file3.txt contents"));
}

// ─── Test: malformed blob ───────────────────────────────────────────────────

void test_malformed_blob_deserialization() {
    // Missing null separator
    ASSERT_THROWS(Blob::deserialize("blob 5abcde"), MiniGitException);

    // Wrong type prefix
    ASSERT_THROWS(Blob::deserialize(std::string("tree 3\0abc", 10)), MiniGitException);

    // Size mismatch
    ASSERT_THROWS(Blob::deserialize(std::string("blob 999\0abc", 12)), MiniGitException);
}

// ─── Test: empty blob ───────────────────────────────────────────────────────

void test_empty_blob() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    Blob blob("");
    std::string id = store.writeBlob(blob);

    Blob restored = store.readBlob(id);
    ASSERT_EQ(restored.content(), std::string(""));
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== MiniGit Object Store Tests ===\n\n";

    RUN_TEST(test_blob_roundtrip);
    RUN_TEST(test_blob_serialization_format);
    RUN_TEST(test_same_content_same_id);
    RUN_TEST(test_different_content_different_id);
    RUN_TEST(test_write_read_blob);
    RUN_TEST(test_read_nonexistent);
    RUN_TEST(test_exists);
    RUN_TEST(test_corrupted_object_detected);
    RUN_TEST(test_multiple_blobs);
    RUN_TEST(test_malformed_blob_deserialization);
    RUN_TEST(test_empty_blob);

    TEST_REPORT();
}
