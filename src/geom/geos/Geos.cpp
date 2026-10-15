// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef GEODESK_WITH_GEOS
#include <geodesk/geom/Distance.h>
#include <geodesk/geom/geos/Geos.h>
#include <geos/operation/distance/DistanceOp.h>

namespace geodesk {

bool Geos::centroid(geos::geom::GeometryFactory& context,
    const geos::geom::Geometry& geom, Coordinate* centroid)
{
    std::unique_ptr<geos::geom::Point> c = geom.getCentroid();
    if (!c || c->isEmpty()) return false;
    *centroid = Coordinate(c->getX(), c->getY());
    return true;
}

double Geos::distanceMeters(geos::geom::GeometryFactory& context,
    const geos::geom::Geometry& geom1, const geos::geom::Geometry& geom2)
{
    auto nearestPoints = geos::operation::distance::DistanceOp::nearestPoints(&geom1, &geom2);
    if (!nearestPoints || nearestPoints->size() < 2) [[unlikely]] return -1;
    double x1 = nearestPoints->getX(0);
    double y1 = nearestPoints->getY(0);
    double x2 = nearestPoints->getX(1);
    double y2 = nearestPoints->getY(1);

    return Distance::metersBetween(x1, y1, x2, y2);
}



} // namespace geodesk

#endif // GEODESK_WITH_GEOS
