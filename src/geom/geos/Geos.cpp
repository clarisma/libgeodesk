// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef GEODESK_WITH_GEOS
#include <geodesk/geom/Distance.h>
#include <geodesk/geom/geos/Geos.h>

namespace geodesk {

bool Geos::centroid(GEOSContextHandle_t context,
    const GEOSGeometry* geom, Coordinate* centroid)
{
    double x, y;
    GEOSGeometry* c = GEOSGetCentroid_r(context, geom);
    if (!c) return false;
    GEOSGeomGetX_r(context, c, &x);
    GEOSGeomGetY_r(context, c, &y);
    GEOSGeom_destroy_r(context, c);
    *centroid = Coordinate(x, y);
    return true;
}

double Geos::distanceMeters(GEOSContextHandle_t context,
    const GEOSGeometry* geom1, const GEOSGeometry* geom2)
{

    GEOSCoordSequence *nearestPoints = GEOSNearestPoints_r
        (context, geom1, geom2);
    if (!nearestPoints) [[unlikely]] return -1;

    double d;
    double x1, x2, y1, y2;
    if (GEOSCoordSeq_getXY_r(context, nearestPoints, 0, &x1, &y1) &&
        GEOSCoordSeq_getXY_r(context, nearestPoints, 1, &x2, &y2))
        [[likely]]
    {
        d = Distance::metersBetween(x1,y1,x2,y2);
    }
    else
    {
        d = -1;
    }
    GEOSCoordSeq_destroy(nearestPoints);
    return d;
}



} // namespace geodesk

#endif // GEODESK_WITH_GEOS

