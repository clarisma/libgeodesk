// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/export.h>
#include <geodesk/feature/forward.h>
#include <geodesk/geom/Coordinate.h>

namespace geodesk {

class Filter;

/// \cond lowlevel
class GEODESK_API Filters
{
public:
    static const Filter* intersects(Feature feature);
    static const Filter* within(Feature feature);
    static const Filter* containsPoint(Coordinate xy);
    static const Filter* crossing(Feature feature);
    static const Filter* maxMetersFrom(double meters, Coordinate xy);
};

// \endcond

} // namespace geodesk
