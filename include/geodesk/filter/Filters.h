// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <span>
#include <geodesk/export.h>
#include <geodesk/feature/forward.h>
#include <geodesk/geom/Coordinate.h>
#ifdef GEODESK_WITH_GEOS
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Geometry.h>
#endif

namespace geodesk {

class Filter;
class StringTable;

/// \cond lowlevel
class GEODESK_API Filters
{
public:
    static const Filter* intersects(Feature feature);
    static const Filter* within(Feature feature);
    static const Filter* containsPoint(Coordinate xy);
    static const Filter* crossing(Feature feature);
#ifdef GEODESK_WITH_GEOS
    static const Filter* intersecting(geos::geom::GeometryFactory& context, const geos::geom::Geometry& geom);
    static const Filter* within(geos::geom::GeometryFactory& context, const geos::geom::Geometry& geom);
    static const Filter* containing(geos::geom::GeometryFactory& context, const geos::geom::Geometry& geom);
    static const Filter* crossing(geos::geom::GeometryFactory& context, const geos::geom::Geometry& geom);
#endif
    static const Filter* maxMetersFrom(double meters, Coordinate xy);
    static const Filter* withRole(std::span<const std::string_view> roles, const StringTable& strings);
};

// \endcond

} // namespace geodesk
