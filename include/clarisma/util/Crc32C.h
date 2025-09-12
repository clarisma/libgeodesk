// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

// CRC32C (Castagnoli, 0x1EDC6F41) with HW fast paths:
// - x86-64: SSE4.2 _mm_crc32_*
// - ARM64 : __crc32c*
// Software fallback uses a 256-entry table (in Crc32C.cpp).
// Hybrid: hot loops inline here; dispatch & table in the .cpp.
//
// Conventions:
// - init = 0xFFFFFFFF, final = bitwise NOT (~)
// - "123456789" -> 0xE3069283
//
// Thread-safety:
// - Dispatch is resolved before main() via TU-local initializer.
// - No per-call feature checks, no locks.

#if defined(__x86_64__) || defined(_M_X64)
    #include <nmmintrin.h>  // _mm_crc32_*
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
    #if defined(_MSC_VER)
        #include <intrin.h>
    #else
        #include <arm_acle.h>
    #endif
#endif

namespace clarisma
{

bool testCrc32C(const void* p, size_t len);

/// @brief CRC32C (Castagnoli) checksum engine.
/// @details
/// One-time dispatch at startup selects the best raw implementation.
/// Calls only perform an indirect call (no extra branch).
///
class Crc32C
{
public:
    Crc32C(uint32_t seed = 0xFFFFFFFFu) : value_(seed) {}

    /// @brief Compute CRC32C over a buffer.
    /// @param data Pointer to bytes
    /// @param len  Number of bytes
    /// @param seed Initial CRC (default ~0)
    /// @return Finalized CRC32C (after ~)
    static uint32_t compute(const void* data,
        size_t len, uint32_t seed = 0xFFFFFFFFu) noexcept
    {
        return finalize(rawFn_(data, len, seed));
    }

    /// @brief Update a running CRC32C (no final XOR).
    /// @param seed Current CRC (use ~0 for first chunk)
    /// @param data Pointer to bytes
    /// @param len  Number of bytes
    /// @return Updated CRC (not finalized)
    void update(const void* data, size_t len) noexcept
    {
        value_ = rawFn_(data, len, value_);
    }

    uint32_t get() const noexcept
    {
        return finalize(value_);
    }

    /// @brief Finalize a running CRC32C (apply final XOR).
    static uint32_t finalize(uint32_t crc) noexcept
    {
        return ~crc;
    }

protected:
    /// @brief Raw CRC32C using SSE4.2 (no final XOR).
    static inline uint32_t x86Raw(const void* data,
                                  size_t n,
                                  uint32_t crc) noexcept
    {
    #if defined(__x86_64__) || defined(_M_X64)
        const uint8_t* p = static_cast<const uint8_t*>(data);

        while (n >= 8)
        {
            uint64_t v;
            std::memcpy(&v, p, 8);
            crc = static_cast<uint32_t>(_mm_crc32_u64(crc, v));
            p += 8;
            n -= 8;
        }
        if (n >= 4)
        {
            uint32_t v;
            std::memcpy(&v, p, 4);
            crc = _mm_crc32_u32(crc, v);
            p += 4;
            n -= 4;
        }
        if (n >= 2)
        {
            uint16_t v;
            std::memcpy(&v, p, 2);
            crc = _mm_crc32_u16(crc, v);
            p += 2;
            n -= 2;
        }
        if (n)
        {
            crc = _mm_crc32_u8(crc, *p);
        }
        return crc;
    #else
        (void)data; (void)n; (void)crc;
        return softRaw(data, n, crc);
    #endif
    }

    /// @brief Raw CRC32C using ARMv8 CRC (no final XOR).
    static inline uint32_t armRaw(const void* data,
                                  size_t n,
                                  uint32_t crc) noexcept
    {
    #if defined(__aarch64__) || defined(_M_ARM64)
        const uint8_t* p = static_cast<const uint8_t*>(data);

        while (n >= 8)
        {
            uint64_t v;
            std::memcpy(&v, p, 8);
            crc = __crc32cd(crc, v);
            p += 8;
            n -= 8;
        }
        if (n >= 4)
        {
            uint32_t v;
            std::memcpy(&v, p, 4);
            crc = __crc32cw(crc, v);
            p += 4;
            n -= 4;
        }
        if (n >= 2)
        {
            uint16_t v;
            std::memcpy(&v, p, 2);
            crc = __crc32ch(crc, v);
            p += 2;
            n -= 2;
        }
        if (n)
        {
            crc = __crc32cb(crc, *p);
        }
        return crc;
    #else
        (void)data; (void)n; (void)crc;
        return softRaw(data, n, crc);
    #endif
    }

private:
    // --- Implemented in Crc32C.cpp -------------------------------------

    /// @brief Software fallback (table-driven, no final XOR).
    static uint32_t softRaw(const void* data,
                            size_t len,
                            uint32_t seed) noexcept;

    /// @brief Selected raw implementation (set before main()).
    using RawFn = uint32_t (*)(const void*, size_t, uint32_t) noexcept;

    /// @brief Function pointer published by TU-local initializer.
    static RawFn rawFn_;
    friend struct Crc32C_Init;

    uint32_t value_;

    friend bool testCrc32C(const void*, size_t);
};

} // namespace clarisma
