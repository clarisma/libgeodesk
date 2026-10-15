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
/// shift register. Values that straddle a word boundary are handled in
/// the rare refill path. The hot path contains one well-predicted branch;
/// selection between bit and trit decoding is branch-free.
///
/// Requirements:
/// - `end - start` must be >= 8 and a multiple of 8, so that the
///   final bits can always be fetched with one 64-bit read at `end - 8`
/// - Pointers may be unaligned
/// - Target must be 64-bit little-endian
///
/// The logical contents and length of the stream are not known to the
/// reader. It is the caller's responsibility to stop after the last valid
/// value. Calls to next() and nextBit() share a single bit cursor; reading
/// a bit may consume part of an encoded trit.
///
/// A call to next() or nextBit() that would consume data at or beyond `end`
/// throws `std::runtime_error`.
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

    /// \brief Returns the next trit, or the next encoded bit.
    ///
    /// \param asSingleBit If true, consumes and returns one encoded bit.
    ///     Otherwise, consumes and returns one decoded trit.
    /// \return A bit from 0 to 1, or a trit from 0 to 2.
    ///
    /// \throws std::runtime_error if the requested value extends beyond `end`.
    ///
    int next(bool asSingleBit = false)
    {
        unsigned v = buf_ & 3;
        unsigned single = (v & 1) | static_cast<unsigned>(asSingleBit);
        unsigned width = 2 - single;
        unsigned shift = width;

        if (avail_ < width) [[unlikely]]
        {
            unsigned pastAvail = avail_;
            refill();
            unsigned vUpper = buf_ & 3;

            // pastAvail is either zero or one, so it is also the mask for
            // the valid low bits of v.
            v = (v & pastAvail) | (vUpper << pastAvail);
            single = (v & 1) | static_cast<unsigned>(asSingleBit);
            width = 2 - single;
            shift = width - pastAvail;
        }

        avail_ -= shift;
        buf_ >>= shift;
        return static_cast<int>(v & (width | 1));
    }

    /// \brief Returns the next bit (0 or 1).
    ///
    /// \throws std::runtime_error if decoding the next bit would
    ///     require reading at or beyond `end`
    ///
    int nextBit()
    {
        return next(true);
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

    DataPtr p_;         ///< Next refill address (start <= p_ <= end_)
    DataPtr end_;       ///< One past the last readable byte
    uint64_t buf_;      ///< Shift register; next code starts at bit 0
    unsigned avail_;    ///< Number of unconsumed bits in buf_
};

} // namespace clarisma
