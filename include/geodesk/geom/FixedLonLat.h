// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <functional> // for std::hash
#include <cstdint>
#include <string>
#include <cstring>
#include <cmath>
#include <geodesk/geom/Coordinate.h>
#include <geodesk/geom/Mercator.h>
#include <clarisma/math/Math.h>
#include <clarisma/text/Format.h>

namespace geodesk {

/// @brief A WGS-84 coordinate pair in 100-nanodegree fixed-point precision.
///
class FixedLonLat
{
public:
    FixedLonLat() = default;

    explicit constexpr FixedLonLat(int32_t lon100n, int32_t lat100n) :
        lon_(lon100n),
        lat_(lat100n)
    {
    }

    explicit FixedLonLat(double lon, double lat) :
        lon_(clarisma::Math::roundFastToInt32(lon * 1e7)),
        lat_(clarisma::Math::roundFastToInt32(lat * 1e7))
    {
    }

    FixedLonLat(Coordinate c) :
        FixedLonLat(Mercator::lonFromX(c.x), Mercator::latFromY(c.y))
    {
    }

    double lon() const noexcept { return lon_ / 1e7; }
    double lat() const noexcept { return lat_ / 1e7; }
    int32_t lon100nd() const noexcept { return lon_; }
    int32_t lat100nd() const noexcept { return lat_; }

    char* format(char* buf, int prec = 7) const
    {
        char* p = clarisma::Format::formatDouble(buf, lon(), prec);
        *p++ = ',';
        return clarisma::Format::formatDouble(p, lat(), prec);
    }

    template<typename Stream>
    void format(Stream& out) const
    {
        char buf[32];
        char* end = format(buf);
        out.write(buf, end-buf);
    }

    std::string toString() const
    {
        char buf[32];
        format(buf);
        return std::string(buf);
    }

    bool operator==(const FixedLonLat&) const noexcept = default;
    bool operator!=(const FixedLonLat& other) const noexcept = default;

private:
    int32_t lon_;
    int32_t lat_;
};

template<typename Stream>
Stream& operator<<(Stream& out, const FixedLonLat& lonlat)
{
    lonlat.format(out);
    return out;
}

} // namespace geodesk

namespace std {

template<>
struct hash<geodesk::FixedLonLat>
{
    std::size_t operator()(const geodesk::FixedLonLat& pos) const noexcept
    {
        // Simple but effective combination of two int32_t values
        std::size_t h1 = pos.lon100nd();
        std::size_t h2 = pos.lat100nd();
        // Combine the hashes (like boost::hash_combine)
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

} // namespace std

