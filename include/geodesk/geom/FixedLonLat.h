// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstdint>
#include <string>
#include <cstring>
#include <cmath>
#include <geodesk/geom/Coordinate.h>
#include <geodesk/geom/Mercator.h>
#include <clarisma/math/Decimal.h>
#include <clarisma/text/Format.h>

namespace geodesk {

/// @brief A WGS-84 coordinate pair in 100-nanodegree fixed-point precision.
///
class FixedLonLat
{
public:
    FixedLonLat() = default;

    FixedLonLat(int32_t lon100n, int32_t lat100n) :
        lon_(lon100n),
        lat_(lat100n)
    {
    }

    FixedLonLat(double lon, double lat) :
        lon_(static_cast<int32_t>(std::round(lon * 1e7))),
        lat_(static_cast<int32_t>(std::round(lat * 1e7)))
    {
    }

    FixedLonLat(Coordinate c) :
        lon_(static_cast<int32_t>(
            std::round(Mercator::lonFromX(c.x) * 1e7))),
        lat_(static_cast<int32_t>(
            std::round(Mercator::latFromY(c.y) * 1e7)))
    {
    }

    double lon() const noexcept { return lon_ / 1e7; }
    double lat() const noexcept { return lat_ / 1e7; }

    char* format(char* buf, int prec = 7) const
    {
        char* p = clarisma::Format::formatDouble(buf, lon(), prec);
        *p++ = ',';
        return clarisma::Format::formatDouble(p, lat(), prec);
    }

    std::string toString() const
    {
        char buf[32];
        format(buf);
        return std::string(buf);
    }

private:
    int32_t lon_;
    int32_t lat_;
};

template<typename Stream>
Stream& operator<<(Stream& out, const FixedLonLat& lonlat)
{
    char buf[32];
    lonlat.format(buf);
    out.write(buf, std::strlen(buf));
    return out;
}

} // namespace geodesk
