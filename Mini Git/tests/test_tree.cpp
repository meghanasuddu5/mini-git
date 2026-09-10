/// Tests for Tree objects and TreeBuilder (Phase 4).
///
/// Test plan:
///   1. Tree serialization/deserialization roundtrip
///   2. Entries are sorted by name (deterministic)
///   3. Same entries produce the same tree hash
///   4. Different entries produce different hashes
///   5. TreeBuilder with flat files only
///   6. TreeBuilder with nested directories
///   7. TreeBuilder with deeply nested directories
///   8. Unchanged directory produces same tree hash
///   9. findEntry works
///  10. Empty tree builder throws

#include "test_utils.h"
#include "minigit/object/tree.h"
#include "minigit/object/object_store.h"
#include "minigit/object/hash.h"
#include "minigit/utils/error.h"
#include "minigit/utils/filesystem.h"

using namespace minigit;

// ─── Test: Tree serialization roundtrip ─────────────────────────────────────

void test_tree_roundtrip() {
    std::vector<TreeEntry> entries = {
        {"100644", ObjectType::Blob, "aaaa" + std::string(60, 'a'), "file_b.txt"},
        {"100644", ObjectType::Blob, "bbbb" + std::string(60, 'b'), "file_a.txt"},
    };

    Tree original(entries);
    std::string serialized = original.serialize();
    Tree restored = Tree::deserialize(serialized);

    // Entries should be sorted by name after construction
    ASSERT_EQ(restored.entries().size(), static_cast<size_t>(2));
    ASSERT_EQ(restored.entries()[0].name, std::string("file_a.txt"));
    ASSERT_EQ(restored.entries()[1].name, std::string("file_b.txt"));
}

// ─── Test: entries are sorted deterministically ─────────────────────────────

void test_tree_sorted_entries() {
    std::string hash1 = std::string(64, '1');
    std::string hash2 = std::string(64, '2');
    std::string hash3 = std::string(64, '3');

    // Insert in reverse order
    std::vector<TreeEntry> entries = {
        {"100644", ObjectType::Blob, hash3, "zebra.txt"},
        {"100644", ObjectType::Blob, hash1, "alpha.txt"},
        {"100644", ObjectType::Blob, hash2, "middle.txt"},
    };

    Tree tree(entries);

    ASSERT_EQ(tree.entries()[0].name, std::string("alpha.txt"));
    ASSERT_EQ(tree.entries()[1].name, std::string("middle.txt"));
    ASSERT_EQ(tree.entries()[2].name, std::string("zebra.txt"));
}

// ─── Test: same entries produce same hash ───────────────────────────────────

void test_tree_deterministic_hash() {
    std::string hash = std::string(64, 'a');

    // Create two trees with same entries but different insertion order
    Tree tree1(std::vector<TreeEntry>{
        {"100644", ObjectType::Blob, hash, "b.txt"},
        {"100644", ObjectType::Blob, hash, "a.txt"},
    });

    Tree tree2(std::vector<TreeEntry>{
        {"100644", ObjectType::Blob, hash, "a.txt"},
        {"100644", ObjectType::Blob, hash, "b.txt"},
    });

    // Serialization should be identical
    ASSERT_EQ(tree1.serialize(), tree2.serialize());

    // Therefore hashes should match
    ASSERT_EQ(SHA256::hashString(tree1.serialize()),
              SHA256::hashString(tree2.serialize()));
}

// ─── Test: different entries produce different hashes ────────────────────────

void test_tree_different_hashes() {
    Tree tree1(std::vector<TreeEntry>{
        {"100644", ObjectType::Blob, std::string(64, 'a'), "file.txt"},
    });

    Tree tree2(std::vector<TreeEntry>{
        {"100644", ObjectType::Blob, std::string(64, 'b'), "file.txt"},
    });

    ASSERT_NE(SHA256::hashString(tree1.serialize()),
              SHA256::hashString(tree2.serialize()));
}

// ─── Test: TreeBuilder with flat files ──────────────────────────────────────

void test_builder_flat_files() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    // First, store some blobs
    Blob blob1("content of file1");
    Blob blob2("content of file2");
    std::string hash1 = store.writeBlob(blob1);
    std::string hash2 = store.writeBlob(blob2);

    // Build a tree with flat files only
    TreeBuilder builder;
    builder.addEntry("file1.txt", hash1);
    builder.addEntry("file2.txt", hash2);

    std::string treeHash = builder.build(store);
    ASSERT_EQ(treeHash.size(), static_cast<size_t>(64));

    // Read back the tree
    auto raw = store.readRaw(treeHash);
    ASSERT_TRUE(raw.has_value());

    Tree tree = Tree::deserialize(raw.value());
    ASSERT_EQ(tree.entries().size(), static_cast<size_t>(2));
    ASSERT_EQ(tree.entries()[0].name, std::string("file1.txt"));
    ASSERT_EQ(tree.entries()[1].name, std::string("file2.txt"));
    ASSERT_EQ(tree.entries()[0].hash, hash1);
    ASSERT_EQ(tree.entries()[1].hash, hash2);
}

// ─── Test: TreeBuilder with nested directories ──────────────────────────────

void test_builder_nested_dirs() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    Blob blob1("readme content");
    Blob blob2("main content");
    Blob blob3("util content");
    std::string h1 = store.writeBlob(blob1);
    std::string h2 = store.writeBlob(blob2);
    std::string h3 = store.writeBlob(blob3);

    TreeBuilder builder;
    builder.addEntry("README.md", h1);
    builder.addEntry("src/main.cpp", h2);
    builder.addEntry("src/utils.h", h3);

    std::string rootHash = builder.build(store);

    // Read the root tree
    Tree rootTree = Tree::deserialize(store.readRaw(rootHash).value());
    ASSERT_EQ(rootTree.entries().size(), static_cast<size_t>(2));

    // First entry: README.md (blob)
    ASSERT_EQ(rootTree.entries()[0].name, std::string("README.md"));
    ASSERT_EQ(static_cast<int>(rootTree.entries()[0].type),
              static_cast<int>(ObjectType::Blob));

    // Second entry: src (tree)
    ASSERT_EQ(rootTree.entries()[1].name, std::string("src"));
    ASSERT_EQ(static_cast<int>(rootTree.entries()[1].type),
              static_cast<int>(ObjectType::Tree));

    // Read the src/ subtree
    Tree srcTree = Tree::deserialize(
        store.readRaw(rootTree.entries()[1].hash).value());
    ASSERT_EQ(srcTree.entries().size(), static_cast<size_t>(2));
    ASSERT_EQ(srcTree.entries()[0].name, std::string("main.cpp"));
    ASSERT_EQ(srcTree.entries()[1].name, std::string("utils.h"));
}

// ─── Test: deeply nested directories ────────────────────────────────────────

void test_builder_deep_nesting() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    Blob blob("deep file content");
    std::string blobHash = store.writeBlob(blob);

    TreeBuilder builder;
    builder.addEntry("a/b/c/d/deep.txt", blobHash);
    builder.addEntry("a/top.txt", blobHash);

    std::string rootHash = builder.build(store);

    // Root should have one entry: "a" (tree)
    Tree root = Tree::deserialize(store.readRaw(rootHash).value());
    ASSERT_EQ(root.entries().size(), static_cast<size_t>(1));
    ASSERT_EQ(root.entries()[0].name, std::string("a"));
    ASSERT_EQ(static_cast<int>(root.entries()[0].type),
              static_cast<int>(ObjectType::Tree));

    // "a" should have "b" (tree) and "top.txt" (blob)
    Tree aTree = Tree::deserialize(
        store.readRaw(root.entries()[0].hash).value());
    ASSERT_EQ(aTree.entries().size(), static_cast<size_t>(2));
    ASSERT_EQ(aTree.entries()[0].name, std::string("b"));
    ASSERT_EQ(aTree.entries()[1].name, std::string("top.txt"));
}

// ─── Test: unchanged directory produces same hash ───────────────────────────

void test_unchanged_directory_same_hash() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    Blob blob1("file content 1");
    Blob blob2("file content 2");
    std::string h1 = store.writeBlob(blob1);
    std::string h2 = store.writeBlob(blob2);

    // Build tree twice with same entries
    TreeBuilder builder1;
    builder1.addEntry("a.txt", h1);
    builder1.addEntry("b.txt", h2);
    std::string hash1 = builder1.build(store);

    TreeBuilder builder2;
    builder2.addEntry("a.txt", h1);
    builder2.addEntry("b.txt", h2);
    std::string hash2 = builder2.build(store);

    ASSERT_EQ(hash1, hash2);
}

// ─── Test: findEntry ────────────────────────────────────────────────────────

void test_find_entry() {
    std::string hash = std::string(64, 'a');

    Tree tree(std::vector<TreeEntry>{
        {"100644", ObjectType::Blob, hash, "found.txt"},
        {"040000", ObjectType::Tree, hash, "subdir"},
    });

    auto* found = tree.findEntry("found.txt");
    ASSERT_TRUE(found != nullptr);
    ASSERT_EQ(found->name, std::string("found.txt"));

    auto* notFound = tree.findEntry("missing.txt");
    ASSERT_TRUE(notFound == nullptr);
}

// ─── Test: empty builder throws ─────────────────────────────────────────────

void test_empty_builder_throws() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    TreeBuilder builder;
    ASSERT_THROWS(builder.build(store), MiniGitException);
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== MiniGit Tree Tests ===\n\n";

    RUN_TEST(test_tree_roundtrip);
    RUN_TEST(test_tree_sorted_entries);
    RUN_TEST(test_tree_deterministic_hash);
    RUN_TEST(test_tree_different_hashes);
    RUN_TEST(test_builder_flat_files);
    RUN_TEST(test_builder_nested_dirs);
    RUN_TEST(test_builder_deep_nesting);
    RUN_TEST(test_unchanged_directory_same_hash);
    RUN_TEST(test_find_entry);
    RUN_TEST(test_empty_builder_throws);

    TEST_REPORT();
}
