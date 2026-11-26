// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/filter/SpatialFilter.h>
#include <geodesk/feature/NodePtr.h>

namespace geodesk {

/// A Filter that selects all features whose bounding box
/// intersect the given bounds. Useful for node, member and parent
/// queries, which don't use a bounding box natively and hence must
/// must rely on a Filter.
///
/// \cond lowlevel
///
class BoundsFilter : public SpatialFilter
{
public:
    explicit BoundsFilter(const Box& bounds) :
        SpatialFilter(bounds) {}

    bool accept(FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const override
    {
        if (feature.isNode())
        {
            NodePtr node(feature);
            return bounds_.contains(node.xy());
        }
        return feature.intersects(bounds_);
    }
};

// \endcond
} // namespace geodesk
