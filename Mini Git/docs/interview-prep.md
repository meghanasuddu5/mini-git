# MiniGit — Technical Interview Preparation & Architecture Deep Dive

This document provides a comprehensive breakdown of the core concepts, data structures, algorithms, and system design decisions implemented in **MiniGit**. It is designed as a study guide and reference for technical interviews involving systems engineering, data structures, and version control internals.

---

## 1. Core Data Structures & The Object Model

### 1.1 Content-Addressable Storage (CAS)
**Question:** *What is content-addressable storage, and why is it used in version control systems?*

- **Definition:** In content-addressable storage, data is retrieved based on the cryptographic hash of its contents rather than an arbitrary filename or location.
- **Implementation in MiniGit:** Every object (Blob, Tree, Commit) is hashed using SHA-256 over its serialized byte stream (e.g., `"blob <size>\0<content>"`).
- **Key Advantages:**
  1. **Automatic Deduplication:** If multiple files across different directories or commits have identical contents, they share the exact same Blob object in `.minigit/objects/`.
  2. **Immutability:** Once written, an object can never be altered without changing its hash.
  3. **Tamper-Evidence & Integrity:** Any corruption or tampering is instantly detectable by recomputing the SHA-256 hash and comparing it to the object ID.

---

### 1.2 Object Types: Blob, Tree, and Commit

```
                   Commit (Hash: 4a8f...)
                    ├── tree: 8c2d... (Root Directory)
                    ├── parent: 1f0e...
                    ├── author: Alice
                    └── message: "Add authentication"
                            │
                            ▼
                    Tree (Hash: 8c2d...)
                    ├── 100644 blob e3b0... README.md
                    └── 040000 tree 9b12... src/
                                    │
                                    ▼
                            Tree (Hash: 9b12...)
                            ├── 100644 blob a948... main.cpp
                            └── 100644 blob 7d86... auth.cpp
```

| Object Type | Responsibilities & Stored Metadata | Why Decoupled? |
| :--- | :--- | :--- |
| **Blob** | Raw file contents only. No filename, no permissions, no timestamps. | Allows maximal deduplication across name changes and directory moves. |
| **Tree** | Ordered list of entries: `<mode> <type> <hash> <name>`. Represents directory snapshots. | Enables hierarchical Merkle trees. Deterministic sorting guarantees identical directory contents always yield the same tree hash. |
| **Commit** | Links a root tree hash, parent commit hash(es), author info, timestamp, and commit message. | Represents points in time in the commit history DAG. |

---

### 1.3 Why Does the Staging Index Exist?
**Question:** *Why does Git/MiniGit use a staging area (index) instead of committing the working tree directly?*

1. **Granular Commit Crafting:** Developers can stage subsets of files or changes, enabling atomic, well-scoped commits.
2. **Performance Cache:** The index caches file metadata and blob hashes, enabling high-speed diffing and status checks without hashing untouched files repeatedly.
3. **Merge Buffer:** During three-way merges, the index coordinates resolution before committing.

---

## 2. Core Algorithms

### 2.1 Line-by-Line Diff: Longest Common Subsequence (LCS)
MiniGit computes differences using dynamic programming to find the Longest Common Subsequence between line sequences:

```cpp
// DP recurrence:
if (oldLines[i - 1] == newLines[j - 1]) {
    dp[i][j] = dp[i - 1][j - 1] + 1;
} else {
    dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
}
```

- **Time Complexity:** $O(M \times N)$ where $M$ and $N$ are the line counts.
- **Space Complexity:** $O(M \times N)$ table with $O(M + N)$ backtracking for diff generation.
- **Output:** Categorizes lines into `Unchanged`, `Added` (`+`), and `Deleted` (`-`).

---

### 2.2 Lowest Common Ancestor (LCA) / Merge Base Finding
**Question:** *How do you find the merge base of two branches in a DAG?*

- **Algorithm in MiniGit (`GraphAnalyzer::findMergeBase`):**
  1. Perform a Backward Breadth-First Search (BFS) starting from `Commit A`, collecting all visited ancestor hashes into a hash set `ancestorsA`.
  2. Perform a Backward BFS starting from `Commit B`.
  3. The first commit dequeued during `Commit B`'s traversal that exists in `ancestorsA` is the Lowest Common Ancestor (closest common point of divergence).
- **Time Complexity:** $O(V + E)$ where $V$ is the number of reachable commits and $E$ is parent pointers.

---

### 2.3 Three-Way Merge Engine
**Question:** *Why is a three-way merge superior to a two-way merge?*

- In a **two-way merge** (comparing only `OURS` vs `THEIRS`), if a file differs between branches, it is impossible to know whether `Branch A` modified it, `Branch B` modified it, or both.
- A **three-way merge** introduces the **BASE** (common ancestor). By comparing both `OURS` and `THEIRS` against `BASE`:
  - If changed in `OURS` but matches `BASE` in `THEIRS` $\rightarrow$ Keep `OURS`.
  - If changed in `THEIRS` but matches `BASE` in `OURS` $\rightarrow$ Take `THEIRS`.
  - If deleted in `THEIRS` and untouched in `OURS` $\rightarrow$ Delete file.
  - If modified differently in both $\rightarrow$ Perform line-by-line merge. If lines conflict, generate conflict markers:
    ```
    <<<<<<< <ourBranch>
    [lines from ours]
    =======
    [lines from theirs]
    >>>>>>> <theirBranch>
    ```

---

## 3. High-Frequency Interview Questions

### Q1: Why is branching in Git $O(1)$?
**Answer:** Branches are not copies of files or trees. A branch is simply a text file under `.minigit/refs/heads/<branch_name>` containing a 64-character hex commit hash. Creating a branch is an $O(1)$ filesystem write of a single pointer.

### Q2: What is a "Detached HEAD"?
**Answer:** Normally, `HEAD` is a symbolic reference pointing to a branch name (`ref: refs/heads/main`). When you checkout a specific commit hash rather than a branch name, `HEAD` points directly to that commit hash. Any new commits made in this state advance `HEAD` but do not belong to any branch.

### Q3: Why does MiniGit use fan-out directories (`objects/ab/cdef...`)?
**Answer:** Storing hundreds of thousands of files in a single flat directory degrades filesystem performance due to directory entry lookup limits ($O(N)$ or hash bucket collisions in standard OS filesystems like ext4/NTFS). Splitting the 64-character hash by its first 2 hex characters creates 256 subdirectories, distributing filesystem inodes evenly.

### Q4: How does Merkle tree hashing ensure data integrity?
**Answer:** In MiniGit's tree model, the hash of a directory tree is computed over the hashes of its child files (blobs) and child directories (subtrees). If even a single bit in a leaf blob changes:
1. That blob's hash changes.
2. The parent tree's serialized entries change, producing a new tree hash.
3. The root tree hash changes.
4. The commit hash changes.
This forms a Merkle tree where the root commit hash cryptographically seals the entire repository state.
