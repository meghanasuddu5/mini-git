# MiniGit 

A lightweight, Git-like version control system built from scratch in standard C++20. 

This project was developed as a deep dive into the internal mechanics of modern version control systems. It implements the complete Git core architecture (content-addressable storage, index, trees, multi-parent commits, branch management, dynamic programming diffs, and 3-way merge resolution) entirely from scratch without external libraries for cryptography or compression.

## 🚀 Features

- **Object Storage (CAS)**: A content-addressable object database supporting Blobs, Trees, and Commits with fan-out directory hashing (`objects/ab/cdef...`).
- **SHA-256 Engine**: A custom SHA-256 hashing engine implemented from the FIPS 180-4 specification (zero external dependencies).
- **Staging Area (Index)**: Tracks modifications, additions, and deletions between the working directory, index, and HEAD commits.
- **Dynamic Programming Diffs**: Line-by-line diff engine powered by the Longest Common Subsequence (LCS) dynamic programming algorithm.
- **Three-Way Merge Engine**: Full 3-way merge resolution using Lowest Common Ancestor (LCA) graph traversal, independent change merging, and conflict marker injection (`<<<<<<<`, `=======`, `>>>>>>>`).
- **Branching & Checkout**: Full support for branch creation, deletion (`-d`), renaming (`-m`), history traversal, and detached HEAD checkouts.
- **.minigitignore**: Regex-based file ignoring system.
- **Performance Benchmarks**: Benchmarking suite measuring SHA-256 hashing throughput, object store I/O, and tree construction.

## 🛠️ Commands Supported

```bash
minigit init                   # Initialize a new repository
minigit status                 # View working tree, staged, and untracked status
minigit add <file>             # Stage a file (or 'minigit add .' for all files)
minigit commit -m "msg"        # Create an immutable snapshot (supporting multi-parent merges)
minigit log [--oneline] [-n N] # Walk commit history DAG
minigit show <hash_or_ref>     # Inspect any object (commit, tree, blob) and view diffs
minigit diff [--staged]        # View line differences (working tree vs index or index vs HEAD)
minigit branch [-d|-m]         # List, create, delete, or rename branches
minigit checkout <ref>         # Switch branches or checkout specific commits (detached HEAD)
minigit merge <branch>         # Perform fast-forward or true 3-way branch merges
```

## 🧠 Architecture & Technical Concepts

For an in-depth guide to the architecture, Merkle trees, and interview preparation, see [docs/interview-prep.md](docs/interview-prep.md).

### 1. The Object Model
- **Blobs**: Store raw, immutable file contents.
- **Trees**: Hierarchical directory snapshots mapping filenames to Blob/Tree hashes.
- **Commits**: Store metadata (Author, Timestamp, Message) and link to a Root Tree and Parent Commit(s).

### 2. Merging & Graph Analysis
- **Lowest Common Ancestor (LCA)**: `GraphAnalyzer` traverses the commit DAG backwards with BFS to locate the common merge base.
- **Three-Way Merge**: `MergeEngine` compares `BASE`, `OURS`, and `THEIRS` to cleanly merge independent modifications and inject conflict markers on overlapping edits.

## 🏗️ Build Instructions

MiniGit is written in standard C++20 and uses CMake.

```bash
# 1. Configure with CMake
cmake -B build -S .

# 2. Build the project
cmake --build build --config Release

# 3. Run the automated test suite (13 test suites)
ctest --test-dir build -C Release --output-on-failure

# 4. Run performance benchmarks
./build/Release/benchmark
```

## 🧪 Test Suite

The project includes 13 test suites covering:
- Repository initialization & path finding (`test_init`)
- CLI dispatch and arguments (`test_cli`)
- SHA-256 FIPS 180-4 test vectors (`test_hash`)
- ObjectStore CRUD and corruption detection (`test_object_store`)
- Index staging and status tracking (`test_index`)
- Deterministic tree hashing (`test_tree`)
- Multi-parent commit serialization (`test_commit`)
- Ignore pattern globbing (`test_ignore`)
- RefManager branch management (`test_ref_manager`)
- Checkout and tree reconstruction (`test_checkout`)
- LCS dynamic programming diffs (`test_diff`)
- Fast-forward & 3-way merges with conflicts (`test_merge`)
- DAG LCA merge base analysis (`test_graph_analyzer`)

## 📄 License
MIT License
