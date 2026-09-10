# MiniGit — Design Decisions

This document records the major design decisions made during the development of MiniGit, explaining the rationale behind them. Understanding *why* a system is built a certain way is crucial for interviews and system design.

## 1. Choice of Language: C++20

**Decision:** The project is written in C++20.

**Rationale:**
- **Performance & Control:** C++ provides the low-level memory and filesystem control needed for a systems project like this, without garbage collection pauses.
- **Modern Features:** C++20 offers `std::filesystem` (which is critical for a VCS), concepts, ranges, and `std::string_view`, allowing us to write much cleaner, safer code compared to older C++ standards.
- **Standard Library:** We use the STL extensively (e.g., `std::unordered_map` for indexing, `std::variant` for types) to minimize external dependencies.

## 2. No External Dependencies (Except standard library)

**Decision:** We avoid third-party libraries (like Boost, libgit2, or OpenSSL) for core logic.

**Rationale:**
- **Educational Value:** The purpose of MiniGit is to learn how a VCS works. Relying on `libgit2` or another library to do the heavy lifting defeats the purpose.
- **Simplicity:** It keeps the build system simple and the codebase easy to compile on any platform.
- **Self-Contained:** We will implement our own simple, portable SHA-256 function rather than linking against a crypto library.

## 3. Storage Format: Raw Text vs. Zlib Compression

**Decision:** We store objects as uncompressed raw files, unlike Git which uses zlib compression.

**Rationale:**
- **Readability:** This allows a student or interviewer to simply `cat` (or read) an object file from the `.minigit/objects/` directory to inspect its contents (e.g., viewing a Tree object's entries).
- **Simplicity:** Adding zlib compression adds complexity and a dependency (or requires writing a deflate algorithm, which is out of scope).
- *Trade-off:* Our repository will consume more disk space than a real Git repo, but for an educational toy, readability trumps storage efficiency.

## 4. Object Types (Blob, Tree, Commit)

**Decision:** We adopt Git's three primary object types.

**Rationale:**
- This model is elegantly simple yet powerful.
- **Blobs** only care about content, enabling deduplication.
- **Trees** provide structure and hierarchical organization.
- **Commits** provide history and metadata.
- This design strongly enforces immutability and content-addressability.

## 5. Branching Strategy: Lightweight References

**Decision:** Branches are just text files in `refs/heads/` containing a commit hash.

**Rationale:**
- **O(1) Branching:** Creating a branch is just writing a 40-character string to a file. It doesn't require copying any code or snapshots.
- **Simplicity:** This directly mirrors Git's implementation and is a fantastic talking point in interviews regarding "Why is branching in Git so fast?".

## 6. Diff Algorithm: Simple Line-Based (LCS)

**Decision:** We will implement a readable, line-based diff using a simple algorithm like Longest Common Subsequence (LCS).

**Rationale:**
- **Scope:** Implementing a highly optimized diff algorithm (like Myers diff or Patience diff) is a project in itself.
- **Goal:** Our goal is to demonstrate *that* snapshots can be compared, and to provide readable output to the user. A basic LCS implementation achieves this while remaining understandable.

## 7. The Index (Staging Area)

**Decision:** The index will be a simple, serializable file mapping paths to their corresponding blob hashes and metadata.

**Rationale:**
- The index acts as a crucial buffer between the working directory and the repository, allowing users to craft commits carefully.
- **Performance:** It caches the state of the working tree, speeding up `status` checks.

## 8. Merge Strategy: Three-Way Merge

**Decision:** We will implement a three-way merge rather than a simpler two-way merge.

**Rationale:**
- **Correctness:** A two-way merge (comparing just the two branch tips) cannot correctly determine *who* changed a file. It doesn't know if a file was added by Branch A or deleted by Branch B.
- **Merge Base:** Finding the lowest common ancestor (the merge base) provides the third point of reference necessary to resolve these ambiguities and correctly identify conflicts.

## 9. Error Handling

**Decision:** We will use C++ exceptions and `std::optional`/`std::expected` (or a variant equivalent if C++23's `std::expected` isn't available) for error handling.

**Rationale:**
- We want robust error handling. A VCS that silently corrupts data or crashes on bad input is unacceptable.
- Expected errors (e.g., "File not found during checkout") will use return types like `std::optional`.
- Fatal errors (e.g., "Filesystem read failure") might throw exceptions to abort operations safely.

## 10. Testing: CTest + Custom Assertions

**Decision:** We use CMake's CTest and custom, lightweight assertion macros instead of a heavy framework like GoogleTest.

**Rationale:**
- See point 2 (Simplicity). It avoids downloading and building external test frameworks.
- Building a simple test runner is a good exercise in itself.

---
*This document will evolve as new design challenges are encountered during implementation.*
