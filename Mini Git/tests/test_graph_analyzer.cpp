/// Tests for GraphAnalyzer (DAG traversal, LCA merge base finding).

#include "test_utils.h"
#include "minigit/object/commit.h"
#include "minigit/object/object_store.h"
#include "minigit/object/tree.h"
#include "minigit/repository/graph_analyzer.h"
#include "minigit/utils/filesystem.h"

using namespace minigit;

void test_graph_analyzer_linear() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    // Initial commit C1
    Commit c1("tree1", std::nullopt, "Author", 1000, "C1");
    std::string h1 = store.writeRaw(c1.serialize());

    // C2 -> C1
    Commit c2("tree2", h1, "Author", 2000, "C2");
    std::string h2 = store.writeRaw(c2.serialize());

    // C3 -> C2
    Commit c3("tree3", h2, "Author", 3000, "C3");
    std::string h3 = store.writeRaw(c3.serialize());

    // Merge base of C2 and C3 should be C2
    auto base23 = GraphAnalyzer::findMergeBase(h2, h3, store);
    ASSERT_TRUE(base23.has_value());
    ASSERT_EQ(base23.value(), h2);

    // Merge base of C1 and C3 should be C1
    auto base13 = GraphAnalyzer::findMergeBase(h1, h3, store);
    ASSERT_TRUE(base13.has_value());
    ASSERT_EQ(base13.value(), h1);

    // Ancestry checks
    ASSERT_TRUE(GraphAnalyzer::isAncestor(h1, h3, store));
    ASSERT_TRUE(GraphAnalyzer::isAncestor(h2, h3, store));
    ASSERT_FALSE(GraphAnalyzer::isAncestor(h3, h1, store));
}

void test_graph_analyzer_divergent_branches() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    //       C1 (Base)
    //      /  \
    //     C2   C3 (Branch A / Branch B)
    //     |    |
    //     C4   C5

    Commit c1("tree1", std::nullopt, "Author", 1000, "C1");
    std::string h1 = store.writeRaw(c1.serialize());

    Commit c2("tree2", h1, "Author", 2000, "C2");
    std::string h2 = store.writeRaw(c2.serialize());

    Commit c4("tree4", h2, "Author", 3000, "C4");
    std::string h4 = store.writeRaw(c4.serialize());

    Commit c3("tree3", h1, "Author", 2500, "C3");
    std::string h3 = store.writeRaw(c3.serialize());

    Commit c5("tree5", h3, "Author", 3500, "C5");
    std::string h5 = store.writeRaw(c5.serialize());

    // Merge base of C4 and C5 is C1
    auto base45 = GraphAnalyzer::findMergeBase(h4, h5, store);
    ASSERT_TRUE(base45.has_value());
    ASSERT_EQ(base45.value(), h1);

    // Merge base of C2 and C3 is C1
    auto base23 = GraphAnalyzer::findMergeBase(h2, h3, store);
    ASSERT_TRUE(base23.has_value());
    ASSERT_EQ(base23.value(), h1);

    // C4 is not ancestor of C5 and vice versa
    ASSERT_FALSE(GraphAnalyzer::isAncestor(h4, h5, store));
    ASSERT_FALSE(GraphAnalyzer::isAncestor(h5, h4, store));
    ASSERT_TRUE(GraphAnalyzer::isAncestor(h1, h4, store));
    ASSERT_TRUE(GraphAnalyzer::isAncestor(h1, h5, store));
}

void test_graph_analyzer_history() {
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    Commit c1("tree1", std::nullopt, "Author", 1000, "C1");
    std::string h1 = store.writeRaw(c1.serialize());

    Commit c2("tree2", h1, "Author", 2000, "C2");
    std::string h2 = store.writeRaw(c2.serialize());

    auto history = GraphAnalyzer::getHistory(h2, store);
    ASSERT_EQ(history.size(), static_cast<size_t>(2));
    ASSERT_EQ(history[0], h2);
    ASSERT_EQ(history[1], h1);
}

int main() {
    std::cout << "=== MiniGit GraphAnalyzer Tests ===\n\n";
    RUN_TEST(test_graph_analyzer_linear);
    RUN_TEST(test_graph_analyzer_divergent_branches);
    RUN_TEST(test_graph_analyzer_history);
    TEST_REPORT();
}
