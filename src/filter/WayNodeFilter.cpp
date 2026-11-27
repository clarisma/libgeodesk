// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/filter/WayNodeFilter.h>
#include <geodesk/feature/WayCoordinateIterator.h>


namespace geodesk {

bool WayNodeFilter::accept(const FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const
{
    assert(feature.isWay());
    WayPtr way(feature);
    // LOG("Checking way/%llu", way.id());
    WayCoordinateIterator iter;
    iter.start(way, 0);
    for (;;)
    {
        Coordinate c = iter.next();
        if (c.isNull())
        {
            // LOG("  Not accepted.");
            return false;
        }
        if (c == coord_) break;
    }
    // LOG("  Accepted (prior to secondary filter)!");
    return !secondaryFilter_ || secondaryFilter_->accept(store, feature, fast);
}

} // geodesk
