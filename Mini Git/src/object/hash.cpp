#include "minigit/object/hash.h"

#include <cstring>
#include <iomanip>
#include <sstream>

namespace minigit {

// ─── SHA-256 Constants ──────────────────────────────────────────────────────
// These are the first 32 bits of the fractional parts of the cube roots
// of the first 64 primes (2..311). Defined by FIPS 180-4.
static constexpr uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// ─── Bit manipulation helpers ───────────────────────────────────────────────
// These are the core operations used in SHA-256 rounds.

/// Right-rotate a 32-bit value by n bits.
static inline uint32_t rotr(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

/// SHA-256 "Ch" function: choose between y and z based on x.
static inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

/// SHA-256 "Maj" function: majority of x, y, z.
static inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

/// SHA-256 big sigma 0.
static inline uint32_t bsig0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

/// SHA-256 big sigma 1.
static inline uint32_t bsig1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

/// SHA-256 small sigma 0 (used in message schedule).
static inline uint32_t ssig0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

/// SHA-256 small sigma 1 (used in message schedule).
static inline uint32_t ssig1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

// ─── Constructor ────────────────────────────────────────────────────────────

SHA256::SHA256()
    : bufferSize_(0)
    , totalBytes_(0)
    , finalized_(false) {
    // Initial hash values: first 32 bits of the fractional parts
    // of the square roots of the first 8 primes (2..19).
    state_[0] = 0x6a09e667;
    state_[1] = 0xbb67ae85;
    state_[2] = 0x3c6ef372;
    state_[3] = 0xa54ff53a;
    state_[4] = 0x510e527f;
    state_[5] = 0x9b05688c;
    state_[6] = 0x1f83d9ab;
    state_[7] = 0x5be0cd19;
}

// ─── Process a single 512-bit block ─────────────────────────────────────────

void SHA256::processBlock(const uint8_t* block) {
    // Step 1: Prepare the message schedule W[0..63].
    // W[0..15] come directly from the block (big-endian).
    // W[16..63] are derived from earlier W values.
    uint32_t W[64];
    for (int i = 0; i < 16; ++i) {
        W[i] = (static_cast<uint32_t>(block[i * 4 + 0]) << 24)
             | (static_cast<uint32_t>(block[i * 4 + 1]) << 16)
             | (static_cast<uint32_t>(block[i * 4 + 2]) << 8)
             | (static_cast<uint32_t>(block[i * 4 + 3]));
    }
    for (int i = 16; i < 64; ++i) {
        W[i] = ssig1(W[i - 2]) + W[i - 7] + ssig0(W[i - 15]) + W[i - 16];
    }

    // Step 2: Initialize working variables with current hash state.
    uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];

    // Step 3: 64 rounds of compression.
    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + bsig1(e) + ch(e, f, g) + K[i] + W[i];
        uint32_t t2 = bsig0(a) + maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    // Step 4: Add compressed chunk to current hash state.
    state_[0] += a;  state_[1] += b;  state_[2] += c;  state_[3] += d;
    state_[4] += e;  state_[5] += f;  state_[6] += g;  state_[7] += h;
}

// ─── Update ─────────────────────────────────────────────────────────────────

void SHA256::update(const void* data, size_t length) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    totalBytes_ += length;

    // If we have leftover bytes in the buffer, try to fill it to 64.
    if (bufferSize_ > 0) {
        size_t toFill = 64 - bufferSize_;
        if (length >= toFill) {
            std::memcpy(buffer_ + bufferSize_, bytes, toFill);
            processBlock(buffer_);
            bytes += toFill;
            length -= toFill;
            bufferSize_ = 0;
        } else {
            std::memcpy(buffer_ + bufferSize_, bytes, length);
            bufferSize_ += length;
            return;
        }
    }

    // Process as many full 64-byte blocks as possible directly.
    while (length >= 64) {
        processBlock(bytes);
        bytes += 64;
        length -= 64;
    }

    // Buffer any remaining bytes.
    if (length > 0) {
        std::memcpy(buffer_, bytes, length);
        bufferSize_ = length;
    }
}

void SHA256::update(std::string_view data) {
    update(data.data(), data.size());
}

// ─── Digest ─────────────────────────────────────────────────────────────────

std::array<uint8_t, SHA256::DIGEST_SIZE> SHA256::digest() {
    if (finalized_) {
        // Return cached result if already finalized
        std::array<uint8_t, DIGEST_SIZE> result;
        for (int i = 0; i < 8; ++i) {
            result[i * 4 + 0] = static_cast<uint8_t>(state_[i] >> 24);
            result[i * 4 + 1] = static_cast<uint8_t>(state_[i] >> 16);
            result[i * 4 + 2] = static_cast<uint8_t>(state_[i] >> 8);
            result[i * 4 + 3] = static_cast<uint8_t>(state_[i]);
        }
        return result;
    }

    // SHA-256 padding:
    // 1. Append bit '1' (0x80 byte)
    // 2. Append zeros until length ≡ 56 (mod 64)
    // 3. Append original length in bits as 64-bit big-endian

    uint64_t totalBits = totalBytes_ * 8;

    // Append 0x80
    uint8_t pad = 0x80;
    update(&pad, 1);

    // Pad with zeros until we're 8 bytes short of a 64-byte boundary
    uint8_t zero = 0x00;
    // We need (bufferSize_ + zeros) to reach 56 mod 64
    // But update() already advanced bufferSize_, so:
    while (bufferSize_ != 56) {
        update(&zero, 1);
    }

    // Append total length in bits (big-endian, 8 bytes)
    uint8_t lengthBytes[8];
    for (int i = 7; i >= 0; --i) {
        lengthBytes[i] = static_cast<uint8_t>(totalBits);
        totalBits >>= 8;
    }
    // Process the final block directly (don't use update, which modifies totalBytes_)
    std::memcpy(buffer_ + 56, lengthBytes, 8);
    processBlock(buffer_);

    finalized_ = true;

    // Convert state to bytes (big-endian)
    std::array<uint8_t, DIGEST_SIZE> result;
    for (int i = 0; i < 8; ++i) {
        result[i * 4 + 0] = static_cast<uint8_t>(state_[i] >> 24);
        result[i * 4 + 1] = static_cast<uint8_t>(state_[i] >> 16);
        result[i * 4 + 2] = static_cast<uint8_t>(state_[i] >> 8);
        result[i * 4 + 3] = static_cast<uint8_t>(state_[i]);
    }
    return result;
}

std::string SHA256::hexDigest() {
    return toHex(digest());
}

// ─── Static Methods ─────────────────────────────────────────────────────────

std::string SHA256::hashString(std::string_view input) {
    SHA256 hasher;
    hasher.update(input);
    return hasher.hexDigest();
}

std::string SHA256::hashBytes(const void* data, size_t length) {
    SHA256 hasher;
    hasher.update(data, length);
    return hasher.hexDigest();
}

std::string SHA256::toHex(const std::array<uint8_t, DIGEST_SIZE>& digest) {
    static constexpr char hexChars[] = "0123456789abcdef";
    std::string result;
    result.reserve(HEX_DIGEST_SIZE);
    for (uint8_t byte : digest) {
        result.push_back(hexChars[byte >> 4]);
        result.push_back(hexChars[byte & 0x0f]);
    }
    return result;
}

} // namespace minigit
