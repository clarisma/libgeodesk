// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/query/Selection.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/feature/FeatureIterator.h>
#include <geodesk/match/Matcher.h>

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
    switch (kind_)
    {
    case EMPTY:
        return true;
    case WAY_NODES:
    case MEMBERS:
        if (!usesMatcherOrFilter()) return false;
        break;
    default:
        break;
    }
    FeatureIterator<Feature> iter(*this);
    return iter == nullptr;

    // TODO: Can likely accelerate for parent views also if they aren't filtered
}

bool Selection::contains(FeaturePtr feature) const noexcept
{
    if (!types_.acceptFlags(feature.flags())) return false;
    if (kind_ == WORLD)
    {
        if (feature.isNode())
        {
            if (!bounds().contains(NodePtr(feature).xy())) return false;
        }
        else
        {
            if (!bounds().intersects(feature.bounds())) return false;
        }
    }
    if (!matcher_->mainMatcher().accept(feature)) return false;
    if (filter_)
    {
        if (!filter_->accept(store(), feature, FastFilterHint())) return false;
    }
    switch (kind_)
    {
    case NODES:
        assert(feature.isNode());
        return WayPtr(relatedFeature()).contains(NodePtr(feature));
    }
}

} // namespace geodesk
