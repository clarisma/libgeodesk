// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/query/SimpleQueryBase.h>
#include <geodesk/geom/Box.h>

namespace geodesk {

/// \cond lowlevel

class SimpleQuery : public SimpleQueryBase<SimpleQuery>
{
protected:
    bool acceptTypes(FeatureTypes types) const   // CRTP override
    {
        return types_ & types;
    }

    bool acceptIndex(FeatureIndexType index, uint32_t keys) const   // CRTP override
    {
        return indexBits_ & keys;
    }

    bool acceptBounds(const Box& bounds) const    // CRTP override
    {
        return bounds_.intersects(bounds);
    }

    bool acceptCoordinate(Coordinate xy) const    // CRTP override
    {
        return bounds_.contains(xy);
    }

    Box bounds_;
    FeatureTypes types_;
    uint32_t indexBits_;

    friend class SimpleQueryBase<SimpleQuery>;
};

// \endcond
} // namespace geodesk
