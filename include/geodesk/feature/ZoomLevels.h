// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/text/Format.h>
#include <clarisma/util/BitIterator.h>
#include <clarisma/util/Bits.h>

namespace geodesk {


///
/// \cond lowlevel
///
class ZoomLevels
{
public:
    ZoomLevels() : levels_(0) {}
    explicit ZoomLevels(uint32_t levels) : levels_(levels) {}

    static const uint32_t DEFAULT = 0b1010101010101;
    using Iterator = clarisma::BitIterator<uint32_t>;

    void add(int level)
    {
        assert(level >= 0 && level <= 12);
        levels_ |= (1 << level);
    }

    int count() const noexcept
    {
        return clarisma::Bits::bitCount(levels_);
    }

    Iterator iter() const noexcept
    {
        return Iterator(levels_);
    }

    bool isValidZoomLevel(int zoom) const noexcept
    {
        return (levels_ & (1 << zoom)) != 0;
    }

    void check() const;

    /**
     * Returns the number of levels that are skipped following the given
     * level (0, 1 or 2 in a valid tile pyramid), or -1 for the highest 
     * zoom level (which by definition does not have child levels).
     */
    int skippedAfterLevel(int zoom) const noexcept
    {
        uint32_t childLevels = levels_ >> (zoom + 1);
        return childLevels ? clarisma::Bits::countTrailingZerosInNonZero(childLevels) : -1;
    }

    /**
     * Returns the highest zoom level below the given zoom level.
     * If zoom is 0, returns 0.
     */
    int parentZoom(int zoom) const noexcept
    {
        uint32_t mask = (1 << zoom) - 1;
        int parentZoom = 31 - clarisma::Bits::countLeadingZerosInNonZero32((levels_ & mask) | 1);
        assert(parentZoom < zoom);
        return parentZoom;
    }

    char* format(char* buf) const
    {
        Iterator it = iter();
        char* p = buf;
        for (;;)
        {
            int zoom = it.next();
            if (zoom < 0) break;
            if (p > buf) *p++ = '/';
            p = clarisma::Format::integer(p, zoom);
        }
        *p = 0;
        return p;
    }

    template<typename Stream>
    void format(Stream& out) const
    {
        char buf[64];
        char* end = format(buf);
        out.write(buf, end-buf);
    }

    std::string toString() const
    {
        char buf[64];
        format(buf);
        return std::string(buf);
    }

    operator uint32_t() const { return levels_; }     // NOLINT implicit conversion

private:
    uint32_t levels_;
};

template<typename Stream>
Stream& operator<<(Stream& out, ZoomLevels levels)
{
    char buf[64];
    levels.format(buf);
    std::string_view sv = buf;
    out.write(sv.data(), sv.size());
    return static_cast<Stream&>(out);
}


// \endcond
} // namespace geodesk
