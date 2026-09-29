// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/filter/Filters.h>
#include <geodesk/feature/FeatureBase.h>
#include <geodesk/feature/QueryException.h>
#include <geodesk/filter/CrossesFilter.h>
#include <geodesk/filter/IntersectsFilter.h>
#include <geodesk/filter/PointDistanceFilter.h>
#include <geodesk/filter/RoleFilter.h>
#include <geodesk/filter/WithinFilter.h>

#include "geodesk/geom/geos/Geos.h"

namespace geodesk {

const Filter* filter(PreparedFilterFactory&& factory, Feature feature)
{
    const Filter* filter;
    if(feature.isAnonymousNode())
    {
        filter = factory.forCoordinate(feature.xy());
    }
    else
    {
        filter = factory.forFeature(feature.store(), feature.ptr());
    }
    if(filter == nullptr)
    {
        throw QueryException("Filter not implemented");
    }
    return filter;
}

const Filter* Filters::intersects(Feature feature)
{
    return filter(IntersectsFilterFactory(), feature);
}

const Filter* Filters::within(Feature feature)
{
    return filter(WithinFilterFactory(), feature);
}

const Filter* Filters::containsPoint(Coordinate xy)
{
    return new ContainsPointFilter(xy);
}

const Filter* Filters::crossing(Feature feature)
{
    return filter(CrossesFilterFactory(), feature);
}

const Filter* Filters::maxMetersFrom(double meters, Coordinate xy)
{
    return new PointDistanceFilter(meters, xy);
}

const Filter* Filters::withRole(std::span<const std::string_view> roles, const StringTable& strings)
{
    return new RoleFilter(roles, strings);
}

#ifdef GEODESK_WITH_GEOS

const Filter* filter(PreparedFilterFactory&& factory,
    geos::geom::GeometryFactory& context, const geos::geom::Geometry& geom)
{
    const Filter* filter = factory.forGeometry(context, geom);
    if(filter == nullptr)
    {
        throw QueryException("Filter not implemented");
    }
    return filter;
}


const Filter* Filters::intersecting(geos::geom::GeometryFactory& context, const geos::geom::Geometry& geom)
{
    return filter(IntersectsFilterFactory(), context, geom);
}

const Filter* Filters::within(geos::geom::GeometryFactory& context, const geos::geom::Geometry& geom)
{
    return filter(WithinFilterFactory(), context, geom);
}

const Filter* Filters::containing(geos::geom::GeometryFactory& context, const geos::geom::Geometry& geom)
{
    Coordinate centroid;
    if (!Geos::centroid(context, geom, &centroid))
    {
        throw QueryException("Unable to calculate centroid");
    }
    return new ContainsPointFilter(centroid);
}

const Filter* Filters::crossing(geos::geom::GeometryFactory& context, const geos::geom::Geometry& geom)
{
    return filter(CrossesFilterFactory(), context, geom);
}
#endif

} // namespace geodesk
