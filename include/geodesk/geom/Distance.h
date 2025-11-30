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
    static double pointsSquared(double x1, double y1, double x2, double y2);

    /**
     * Calculates the square of the distance between a line segment
     * [(x1,y1), (x2, y2)] and a point (px,py)
     *
     * @return distance (in units) squared.
     */
    static double pointSegmentSquared(double x1, double y1, double x2, double y2, double px, double py);

    static double metersBetween(Coordinate p1, Coordinate p2)
    {
        double xDelta = static_cast<double>(p1.x) - p2.x;
        double yDelta = static_cast<double>(p1.y) - p2.y;
        double d = sqrt(xDelta * xDelta + yDelta * yDelta);
        return d * Mercator::metersPerUnitAtY(clarisma::Math::avg(p1.y, p2.y));
    }
};

/// \endcond

} // namespace geodesk
