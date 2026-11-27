// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/WayPtr.h>

#include "geodesk/feature/FeatureNodeIterator.h"

namespace geodesk {

bool WayPtr::contains(const FeatureStore* store, NodePtr node) const noexcept
{
    if (!hasFeatureNodes()) return false;
    if (!bounds().contains(node.xy())) return false;
    FeatureNodeIterator iter(store, *this);
    for (;;)
    {
        NodePtr n = iter.next();
        if (n.isNull()) return false;
    }
}

} // namespace geodesk
