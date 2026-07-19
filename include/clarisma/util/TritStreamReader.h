// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <clarisma/util/DataPtr.h>

namespace clarisma {

/// \brief Decodes a stream of trits (base-3 digits) stored in memory
/// as a variable-length, prefix-free bit code.
///
/// The stream is little-endian at the bit level: bits are consumed from
/// the least-significant bit of each byte upward, and bytes are read in
/// ascending address order. Each trit is encoded as follows (bits listed
/// in the order they are consumed):
///
///     Trit | Bits  | Code value
///     -----|-------|-----------
///      0   | 0, 0  | 0b00 = 0
///      1   | 1     | 0b1  = 1
///      2   | 0, 1  | 0b10 = 2
///
/// Note that the bit pattern of each code, read as an integer, equals
/// the trit value itself, which enables branch-free decoding.
///
/// Data is fetched one unaligned `uint64_t` at a time into a 64-bit
/// shift register; a code that straddles a word boundary is handled
/// explicitly in the (rare) refill path, so the hot path of next()
/// contains a single well-predicted branch and no other conditionals.
///
/// Requirements:
/// - `end - start` must be >= 8 and a multiple of 8, so that the
///   final bits can always be fetched with one 64-bit read at `end - 8`
/// - Pointers may be unaligned
/// - Target must be 64-bit little-endian
///
/// The number of trits in the stream is *not* known to the reader;
/// it is the caller's responsibility to stop after the last valid trit.
/// A call to next() that would consume bits at or beyond `end` throws
/// `std::runtime_error`.
///
class TritStreamReader
{
public:
    /// \brief Creates a reader that decodes trits from the given range.
    ///
    /// \param start pointer to the first byte of encoded trit data
    ///     (may be unaligned)
    /// \param end   pointer one past the last readable byte;
    ///     `end - start` must be >= 8 and a multiple of 8
    ///
    TritStreamReader(const uint8_t* start, const uint8_t* end) noexcept :
        p_(start),
        end_(end),
        buf_(0),
        avail_(0)
    {
        assert(end - start >= 8);
        assert(((end - start) & 7) == 0);
    }

    /// \brief Creates a reader that yields `firstTrit` on the first call
    /// to next(), before any trits stored in memory.
    ///
    /// This adds no persistent state or recurring cost to the decode path.
    ///
    /// \param start     pointer to the first byte of encoded trit data
    ///     (may be unaligned)
    /// \param end       pointer one past the last readable byte;
    ///     `end - start` must be >= 8 and a multiple of 8
    /// \param firstTrit the trit (0 to 2) returned before the first
    ///     stored trit
    ///
    TritStreamReader(const uint8_t* start, const uint8_t* end,
        int firstTrit) noexcept :
        p_(start),
        end_(end),
        buf_(static_cast<uint64_t>(firstTrit)),   // code value == trit value
        avail_(2 - (firstTrit & 1))               // trit 1 is a 1-bit code
    {
        assert(end - start >= 8);
        assert(((end - start) & 7) == 0);
        assert(firstTrit >= 0 && firstTrit <= 2);
    }

    /// \brief Returns the next trit (0 to 2).
    ///
    /// \throws std::runtime_error if decoding the next trit would
    ///     require bits at or beyond `end`
    int next()
    {
        if (avail_ < 2) [[unlikely]]
        {
            if (avail_ == 0)
            {
                refill();
                // continue below
            }
            else
            {
                assert(avail_ == 1);
                unsigned firstBit = buf_ & 1;
                if (firstBit != 0)
                {
                    // A lone 1-bit is a complete code for trit 1
                    // (buf_ is left stale; avail_ == 0 forces a refill)
                    avail_ = 0;
                    return 1;
                }
                // The code straddles a word boundary: its first bit (0)
                // is the last of the current word, its second bit is the
                // first of the next word (refill() throws if there isn't
                // one, which is exactly right: that bit lies past `end`)
                refill();
                const int trit = static_cast<int>((buf_ & 1) << 1);
                buf_ >>= 1;
                avail_ = 63;
                return trit;
            }
        }
        unsigned v = buf_ & 3;
        unsigned width = 2 - (v & 1);
        buf_ >>= width;
        avail_ -= width;
        // width|1 is 1 for a 1-bit code (masks v to trit 1) and
        // 3 for a 2-bit code (passes v through as trit 0 or 2)
        return static_cast<int>(v & (width | 1));
    }

private:
    void refill()
    {
        if (p_ == end_) [[unlikely]]
        {
            throw std::runtime_error(
                "TritStreamReader: attempt to read past end of stream");
        }
        buf_ = p_.getUnsignedLongUnaligned();
        p_ += 8;
        avail_ = 64;
    }

    DataPtr p_;         ///< Next refill address (start_ <= p_ <= end_)
    DataPtr end_;       ///< One past the last readable byte
    uint64_t buf_;      ///< Shift register; next code starts at bit 0
    unsigned avail_;    ///< Number of unconsumed bits in buf_
};

} // namespace clarisma
