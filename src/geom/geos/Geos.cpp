// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef GEODESK_WITH_GEOS
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


} // namespace geodesk

#endif // GEODESK_WITH_GEOS

