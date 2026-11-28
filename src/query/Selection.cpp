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
    case WAY_NODES:
        assert(feature.isNode());
        assert(relatedFeature().isWay());
        return WayPtr(relatedFeature()).contains(store(), NodePtr(feature));
    case MEMBERS:
        assert(relatedFeature().isRelation());
        return RelationPtr(relatedFeature()).contains(store(), feature);
    case PARENTS:
        if (feature.isRelation())
        {
            return RelationPtr(feature).contains(store(), relatedFeature());
        }
        assert(feature.isWay());
        assert(relatedFeature().isNode());
        return WayPtr(feature).contains(store(), NodePtr(relatedFeature()));
    default:
        return true;
    }
}

bool Selection::containsAnonNode(Coordinate xy) const noexcept
{
    if (kind_ != WAY_NODES) return false;
    if (usesMatcher()) return false;
    // TODO: Filters need to be able to accept anon nodes
    assert(relatedFeature().isWay());
    return WayPtr(relatedFeature()).containsAnonNode(xy);
}

} // namespace geodesk
