/// MiniGit Performance Benchmarks (Phase 14).

#include "test_utils.h"
#include "minigit/object/hash.h"
#include "minigit/object/object_store.h"
#include "minigit/object/tree.h"
#include "minigit/object/commit.h"
#include "minigit/utils/diff.h"
#include "minigit/utils/filesystem.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using namespace minigit;

void benchmark_sha256() {
    std::cout << "--- Benchmarking SHA-256 Engine ---\n";

    // Generate 10 MB payload
    const size_t totalBytes = 10 * 1024 * 1024;
    std::string payload(totalBytes, 'x');

    auto start = std::chrono::high_resolution_clock::now();
    std::string hash = SHA256::hashString(payload);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;
    double mbPerSec = (static_cast<double>(totalBytes) / (1024.0 * 1024.0)) / duration.count();

    std::cout << "  Payload: 10.0 MB\n";
    std::cout << "  Hash: " << hash.substr(0, 16) << "...\n";
    std::cout << "  Time: " << std::fixed << std::setprecision(3) << duration.count() * 1000.0 << " ms\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(2) << mbPerSec << " MB/s\n\n";
}

void benchmark_object_store() {
    std::cout << "--- Benchmarking ObjectStore Throughput ---\n";
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    const int numObjects = 500;
    std::vector<std::string> ids;
    ids.reserve(numObjects);

    // Benchmark Writes
    auto startWrite = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numObjects; ++i) {
        Blob blob("Sample blob file content version #" + std::to_string(i) + "\nLine 2\nLine 3\n");
        ids.push_back(store.writeBlob(blob));
    }
    auto endWrite = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> writeDuration = endWrite - startWrite;

    // Benchmark Reads
    auto startRead = std::chrono::high_resolution_clock::now();
    for (const auto& id : ids) {
        Blob b = store.readBlob(id);
        (void)b;
    }
    auto endRead = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> readDuration = endRead - startRead;

    std::cout << "  Objects: " << numObjects << "\n";
    std::cout << "  Write Speed: " << std::fixed << std::setprecision(0) << (numObjects / writeDuration.count()) << " writes/sec\n";
    std::cout << "  Read Speed:  " << std::fixed << std::setprecision(0) << (numObjects / readDuration.count()) << " reads/sec\n\n";
}

void benchmark_tree_builder() {
    std::cout << "--- Benchmarking TreeBuilder Hierarchy Construction ---\n";
    TempDir tmp;
    fs::path objDir = tmp.path() / "objects";
    fs::create_directories(objDir);
    ObjectStore store(objDir);

    const int numFiles = 1000;
    TreeBuilder tb;
    std::string dummyHash(64, 'a');

    for (int i = 0; i < numFiles; ++i) {
        std::string path = "src/module_" + std::to_string(i / 50) + "/sub/file_" + std::to_string(i) + ".cpp";
        tb.addEntry(path, dummyHash);
    }

    auto start = std::chrono::high_resolution_clock::now();
    std::string rootTree = tb.build(store);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;

    std::cout << "  Total Files: " << numFiles << " across 20 subdirectories\n";
    std::cout << "  Root Tree Hash: " << rootTree.substr(0, 16) << "...\n";
    std::cout << "  Build Time: " << std::fixed << std::setprecision(3) << duration.count() * 1000.0 << " ms\n\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "       MiniGit Performance Suite        \n";
    std::cout << "========================================\n\n";

    benchmark_sha256();
    benchmark_object_store();
    benchmark_tree_builder();

    return 0;
}
