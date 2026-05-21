// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/geom/Coordinate.h>

namespace geodesk {

/// \cond lowlevel
///
class Distance
{
public:
    /**
     * Calculates the square of the distance between two points
     * (x1,y1) and (x2, y2).
     *
     * @return distance (in units) squared.
     */
    static double pointsSquared(double x1, double y1, double x2, double y2)
    {
        x1 -= x2;
        y1 -= y2;
        return (x1 * x1 + y1 * y1);
    }

    /**
     * Calculates the distance between two points (x1,y1) and (x2, y2).
     *
     * @return distance (in units)
     */
    static double points(double x1, double y1, double x2, double y2)
    {
        x1 -= x2;
        y1 -= y2;
        return std::sqrt(x1 * x1 + y1 * y1);
    }

    /**
     * Calculates the distance between two coordinates.
     *
     * @return distance (in units)
     */
    static double points(Coordinate p1, Coordinate p2)
    {
        return points(p1.x, p1.y, p2.x, p2.y);
    }


    /**
     * Calculates the square of the distance between a line segment
     * [(x1,y1), (x2, y2)] and a point (px,py)
     *
     * @return distance (in units) squared.
     */
    static double pointSegmentSquared(double x1, double y1, double x2, double y2, double px, double py);

    /// Calculates the distance (in meters) between two
    /// Mercator-projected points.
    ///
    static double metersBetween(Coordinate p1, Coordinate p2)
    {
        double xDelta = static_cast<double>(p1.x) - p2.x;
        double yDelta = static_cast<double>(p1.y) - p2.y;
        double d = sqrt(xDelta * xDelta + yDelta * yDelta);
        return d * Mercator::metersPerUnitAtY(clarisma::Math::avg(p1.y, p2.y));
    }

    /// Calculates the distance (in meters) between two
    /// Mercator-projected points.
    ///
    static double metersBetween(double x1, double y1, double x2, double y2)
    {
        double xDelta = x1 - x2;
        double yDelta = y1 - y2;
        double d = sqrt(xDelta * xDelta + yDelta * yDelta);
        return d * Mercator::metersPerUnitAtY((y1 + y2) / 2);
    }
};

/// \endcond

} // namespace geodesk
