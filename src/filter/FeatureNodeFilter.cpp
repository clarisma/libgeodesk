// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/filter/FeatureNodeFilter.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/feature/FeatureNodeIterator.h>

namespace geodesk {

bool FeatureNodeFilter::accept(FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const
{
    // TODO: Ignore missing tiles; by definition, we cannot find the
    // candidate node in a missing tile
    // LOG("Checking %s...", feature.toString().c_str());
    assert(feature.isWay());
    WayPtr way(feature);
    FeatureNodeIterator iter(store, way.bodyptr(), way.flags(), store->borrowAllMatcher(), nullptr);
    for (;;)
    {
        NodePtr node = iter.next();
        if (node.isNull()) return false;
        if (node.ptr() == node_.ptr()) break;
        // Important: Check exact pointers, not conceptual equality
    }
    return !secondaryFilter_ || secondaryFilter_->accept(store, feature, fast);
}

} // geodesk
