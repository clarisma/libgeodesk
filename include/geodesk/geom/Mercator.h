// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <climits>
#include <cmath>
#include <clarisma/math/Math.h>

namespace geodesk {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/// @brief Methods for working with Mercator-projected coordinates.
/// GeoDesk uses a Pseudo-Mercator projection that projects coordinates
/// onto a square Cartesian plane 2^32 units wide/tall (in essence, the
/// value range fully uses a 32-bit signed int). This projection is
/// compatible with [Web %Mercator EPSG:3857](https://epsg.io/3857),
/// except that instead of meters at the Equator, it uses a made-up
/// unit called "imp" ("integer, Mercator-projected").
///
/// \image html projection.png width=50%
///
class /* GEODESK_API */ Mercator
{
public:
	static constexpr double MAP_WIDTH = 4'294'967'294.9999;
	static constexpr double EARTH_CIRCUMFERENCE = 40'075'016.68558;
    static constexpr double MIN_LAT = -85.0511288;
    static constexpr double MAX_LAT =  85.0511287;
    static constexpr int32_t MIN_LAT_100ND = -850511288;
    static constexpr int32_t MAX_LAT_100ND =  850511287;
    static constexpr int32_t MIN_Y = INT_MIN;
    static constexpr int32_t MAX_Y = INT_MAX - 1;

    /// @brief Converts longitude (in WGS-84 degrees) to a %Mercator X-coordinate.
    ///
    static int32_t xFromLon(double lon) noexcept
    {
        return static_cast<int32_t>(round(MAP_WIDTH * lon / 360.0));
    }

    /// @brief Converts WGS-84 longitude (in 100-nanodegree increments)
    /// to a %Mercator X-coordinate.
    ///
    static int32_t xFromLon100nd(int lon) noexcept
    {
        return xFromLon(static_cast<double>(lon) / 10000000.0);
    }

    /// @brief Converts latitude (in WGS-84 degrees) to a %Mercator Y-coordinate.
    ///
    static int32_t yFromLat(double lat) noexcept
    {
        return static_cast<int32_t>(round(std::log(std::tan((lat + 90.0) 
            * M_PI / 360.0)) * (MAP_WIDTH / 2.0 / M_PI)));
    }

    /// @brief Converts WGS-84 latitude (in 100-nanodegree increments)
    /// to a %Mercator Y-coordinate.
    ///
    static int32_t yFromLat100nd(int lat) noexcept
    {
        return yFromLat(static_cast<double>(lat) / 10000000.0);
    }

    static double roundTo100nd(double deg) noexcept
    {
        constexpr double factor = 1e7;  // 10^7
        return std::round(deg * factor) / factor;
    }

    /// @brief Converts a %Mercator X-coordinate to longitude (in WGS-84 degrees).
    ///
    static double lonFromX(double x) noexcept
    {
        return x * 360.0 / MAP_WIDTH;
    }

    /// @brief Converts a %Mercator X-coordinate to longitude
    /// (in WGS-84 degrees, rounded to 7 digits of precision).
    ///
    static double roundedLonFromX(double x) noexcept
    {
        return roundTo100nd(lonFromX(x));
    }

    /// @brief Converts a %Mercator Y-coordinate to latitude (in WGS-84 degrees).
    ///
    static double latFromY(double y) noexcept
    {
        return std::atan(std::exp(y * M_PI * 2.0 / MAP_WIDTH)) * 360.0 / M_PI - 90.0;
    }

    /// @brief Converts a %Mercator Y-coordinate to latitude
    /// (in WGS-84 degrees, rounded to 7 digits of precision).
    ///
    static double roundedLatFromY(double y) noexcept
    {
        return roundTo100nd(latFromY(y));
    }

    /// @brief Calculates the scaling factor for the given Y-coordinate.
    /// This factor is `1` at the equator and progressively increases
    /// towards the polar regions, and is used to compensate for the
    /// distortion introduced by the %Mercator projection.
    ///
    static double scale(double y) noexcept
    {
        /*
        for (double lat = 0; lat <= 86; lat++)
        {
            LOG("new scale at %f lat = %f", lat, 1 / std::cos(lat * M_PI / 180.0));
            LOG("old scale at %f lat = %f", lat, std::cosh(yFromLat(lat) * 2.0 * M_PI / MAP_WIDTH));
        }
        double latMax = 85.0511;
        LOG("new scale at %f lat = %f", latMax, 1 / std::cos(latMax * M_PI / 180.0));
        LOG("old scale at %f lat = %f", latMax, std::cosh(yFromLat(latMax) * 2.0 * M_PI / MAP_WIDTH));
        */

        // Re Issue #33:
        // The original code is fine, equivalent to this (more expensive) formula
        // up to Mercator maximum:
        // return 1 / std::cos(latFromY(y) * M_PI / 180.0);

        return std::cosh(y * 2.0 * M_PI / MAP_WIDTH);
    }

    /// Calculates the number of meters each Mercator unit
    /// represents at the given Y-coordinate. The Y-coordinate
    /// must be between `INT_MIN` and `INT_MAX` (inclusive),
    /// or the result will be undefined.
    ///
    /// @param y valid Y-coordinate
    ///
    static double metersPerUnitAtY(double y) noexcept
    {
        return EARTH_CIRCUMFERENCE / MAP_WIDTH / scale(y);
    }

    /// Calculates the number of Mercator units represented
    /// by the given distance (in meters), at the given
    /// Y-coordinate.
    ///
    /// The Y-coordinate must be between `INT_MIN` and
    /// `INT_MAX` (inclusive), or the result will be undefined.
    ///
    /// @param meters distance in meters
    /// @param atY valid Y-coordinate
    ///
    static double unitsFromMeters(double meters, double atY) noexcept
    {
        return meters * MAP_WIDTH / EARTH_CIRCUMFERENCE * scale(atY);
    }
};

} // namespace geodesk
