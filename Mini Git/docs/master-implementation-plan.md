# MiniGit Master Implementation Plan

This document outlines the step-by-step implementation strategy for all remaining phases of the MiniGit project (Phases 2 through 16). 

## Core Philosophy
1. **Incremental Implementation:** Each phase builds upon the previous one. We do not skip ahead.
2. **Test-Driven:** Every phase must have accompanying tests (using our custom lightweight CTest framework).
3. **Robustness:** We handle errors gracefully using the custom exception hierarchy introduced in Phase 1.
4. **No External Dependencies:** Core logic uses the C++ standard library. (e.g., custom SHA-256 implementation).

---

## Phase 2 — Content-Addressable Object Storage (CAS)
**Goal:** Implement the ability to hash file contents and store them as immutable Blobs.
- **Components:** 
  - `HashEngine`: A standalone SHA-256 implementation.
  - `ObjectStore`: Reads and writes compressed/raw objects to `.minigit/objects/`.
  - `Blob`: Represents file contents.
- **Key Concepts:** SHA-256 hashing, hex conversion, fan-out directory structure (`objects/ab/cdef...`).
- **Tests:** Same content yields same hash, read/write roundtrips, corruption detection.

## Phase 3 — Index / Staging Area
**Goal:** Implement `minigit add <file>` and `minigit status`.
- **Components:**
  - `Index`: Manages the staging area (reads/writes `.minigit/index`).
  - `IndexEntry`: Represents a staged file (path, blob hash, metadata).
- **Key Concepts:** Serialization of the index file, diffing the working tree against the index.
- **Tests:** Add file, modify tracked file, status output correctness.

## Phase 4 — Tree Objects
**Goal:** Group Blobs into directory structures (Tree objects).
- **Components:**
  - `Tree`: Represents a directory snapshot.
  - `TreeEntry`: Name, mode, and object hash (Blob or another Tree).
  - `TreeBuilder`: Constructs Trees from the Index.
- **Key Concepts:** Recursive data structures, deterministic sorting of tree entries for consistent hashing.
- **Tests:** Deterministic tree creation, nested directory trees.

## Phase 5 — Commits
**Goal:** Implement `minigit commit -m "message"`.
- **Components:**
  - `Commit`: Links a Tree hash, parent Commit hash(es), author, and message.
- **Key Concepts:** Directed Acyclic Graph (DAG), immutable snapshots, updating HEAD/branch refs after commit.
- **Tests:** Initial commit (no parent), subsequent commits, persistence.

## Phase 6 — Log + Show
**Goal:** Implement `minigit log` and `minigit show <commit>`.
- **Components:**
  - Graph traversal logic in the CLI or a dedicated `History` module.
- **Key Concepts:** Walking the DAG backwards via parent pointers, formatting commit metadata.
- **Tests:** Linear history traversal, handling missing commits.

## Phase 7 — Diff Engine
**Goal:** Implement `minigit diff` (working tree vs index, or commit vs commit).
- **Components:**
  - `DiffEngine`: Performs line-based Longest Common Subsequence (LCS) comparison.
- **Key Concepts:** LCS algorithm, generating readable added/removed line output.
- **Tests:** Added lines, removed lines, modified lines, new/deleted files.

## Phase 8 — Checkout
**Goal:** Implement `minigit checkout <commit>`.
- **Components:**
  - `CheckoutEngine`: Restores the working tree to match a specific commit's Tree.
- **Key Concepts:** Safe filesystem updates (aborting if uncommitted changes exist), traversing Trees to recreate files.
- **Tests:** Checkout older commit, return to newest commit, safety aborts on dirty working tree.

## Phase 9 — Branches + HEAD
**Goal:** Implement `minigit branch <name>` and `minigit checkout <branch>`.
- **Components:**
  - `RefManager`: Reads/writes files in `.minigit/refs/heads/` and updates `HEAD`.
- **Key Concepts:** Lightweight branching (just text files containing hashes), detached HEAD state.
- **Tests:** Create/list branches, move HEAD, verify commits advance branch pointers.

## Phase 10 — Merge Base (Graph Analysis)
**Goal:** Find the lowest common ancestor of two commits.
- **Components:**
  - `GraphAnalyzer`: Implements Breadth-First Search (BFS) or Depth-First Search (DFS) on the commit DAG.
- **Key Concepts:** DAG traversal, ancestor relationships.
- **Tests:** Linear history base, branched history base, multiple ancestors.

## Phase 11 — Three-Way Merge
**Goal:** Implement `minigit merge <branch>`.
- **Components:**
  - `MergeEngine`: Uses the Merge Base, OURS, and THEIRS to perform a 3-way file merge.
- **Key Concepts:** Conflict detection, injecting conflict markers (`<<<<<<<`, `=======`, `>>>>>>>`).
- **Tests:** Clean merge, independent changes, conflicting changes.

## Phase 12 — Recovery + Robustness
**Goal:** Ensure the system does not silently corrupt data.
- **Components:** Atomic file writes (write to temp file, then rename).
- **Key Concepts:** Filesystem atomicity, handling interrupted operations, strict error reporting.
- **Tests:** Corrupted object detection, malformed refs.

## Phase 13 — Testing + Engineering Quality
**Goal:** Full pass to solidify the codebase.
- **Action Items:** AddressSanitizer/UBsanitizer integration, memory leak checks, code refactoring.

## Phase 14 — Benchmarking
**Goal:** Measure performance characteristics.
- **Action Items:** Hash large files, object read/write throughput, tree traversal speed.

## Phase 15 — Documentation
**Goal:** Comprehensive `README.md` with usage instructions, architecture diagrams, and design limitations.

## Phase 16 — Interview Preparation
**Goal:** Create `docs/interview-prep.md` containing Q&A covering the core concepts, design decisions, and system architecture for technical interviews.
