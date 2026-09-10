#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace minigit {

/// SHA-256 hash engine.
///
/// Implements the SHA-256 algorithm as specified in FIPS 180-4.
/// This is a self-contained implementation with no external dependencies.
///
/// Usage:
///   // One-shot hashing:
///   std::string hex = SHA256::hashString("Hello world\n");
///
///   // Incremental hashing (for large data):
///   SHA256 hasher;
///   hasher.update(chunk1);
///   hasher.update(chunk2);
///   std::string hex = hasher.hexDigest();
///
/// Why we implement this ourselves:
///   - Avoids linking OpenSSL or another crypto library
///   - Educational: understanding hashing is crucial for CAS
///   - The algorithm is ~100 lines of bit manipulation
class SHA256 {
public:
    /// Size of the output hash in bytes (256 bits = 32 bytes).
    static constexpr size_t DIGEST_SIZE = 32;

    /// Size of the hex string representation (64 characters).
    static constexpr size_t HEX_DIGEST_SIZE = 64;

    SHA256();

    /// Feed data into the hash computation.
    /// Can be called multiple times for incremental hashing.
    void update(const void* data, size_t length);

    /// Convenience: update with a string_view.
    void update(std::string_view data);

    /// Finalize and return the raw 32-byte digest.
    /// After calling this, the hasher should not be reused.
    std::array<uint8_t, DIGEST_SIZE> digest();

    /// Finalize and return the 64-character hex string.
    std::string hexDigest();

    // ── Static convenience methods ──────────────────────────────────

    /// Hash a string and return the hex digest.
    static std::string hashString(std::string_view input);

    /// Hash raw bytes and return the hex digest.
    static std::string hashBytes(const void* data, size_t length);

    /// Convert a raw digest to a hex string.
    static std::string toHex(const std::array<uint8_t, DIGEST_SIZE>& digest);

private:
    /// Process a single 64-byte (512-bit) block.
    void processBlock(const uint8_t* block);

    /// The eight working variables (hash state).
    uint32_t state_[8];

    /// Buffer for incomplete blocks (up to 64 bytes).
    uint8_t buffer_[64];

    /// Number of bytes currently in the buffer.
    size_t bufferSize_;

    /// Total number of bytes processed (for the final padding).
    uint64_t totalBytes_;

    /// Whether digest() has been called (prevents reuse).
    bool finalized_;
};

} // namespace minigit
