// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/query/Selection.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/feature/FeatureIterator.h>

namespace geodesk {

uint64_t Selection::count() const
{
    switch (kind())
    {
    case EMPTY:
        return 0;
    case WAY_NODES:
        if (!usesMatcherOrFilter())
        {
            return WayPtr(relatedFeature()).nodeCount();
        }
        break;
    case WORLD:
    {
        uint64_t count = 0;
        Query query(store_, bounds(), types_, matcher_, filter_);
        while (!query.next().isNull()) count++;
        return count;
    }
    default:
        break;
    }
    uint64_t count = 0;
    FeatureIterator<Feature> iter(*this);
    while (iter != nullptr)
    {
        ++iter;
        ++count;
    }
    return count;
}

bool Selection::isEmpty() const
{
    if(kind_ == EMPTY) return true;
    if (kind_ == WAY_NODES) [[unlikely]]
    {
        if (!usesMatcherOrFilter()) return false;
    }
    FeatureIterator<Feature> iter(*this);
    return iter == nullptr;
}

} // namespace geodesk
