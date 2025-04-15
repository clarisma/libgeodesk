// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/FeatureBase.h>
#include <geodesk/feature/Features.h>
#include <geodesk/feature/Nodes.h>

// \cond

namespace geodesk {

// TODO: Add type constraints to nodes/members

template<typename T>
Nodes FeatureBase<T>::nodes() const
{
    return nodes(nullptr);
}

template<typename T>
Nodes FeatureBase<T>::nodes(const char* query) const
{
    if(isWay())
    {
        return Nodes(View::nodesOf(store(), ptr(), query));
    }
    return Nodes::empty(store());
}

template<typename T>
Features FeatureBase<T>::members() const
{
    return members(nullptr);
}

template<typename T>
Features FeatureBase<T>::members(const char* query) const
{
    if(isRelation())
    {
        return Features(View::membersOf(store(), ptr(), query));
    }
    if(isWay())
    {
        return Features(View::nodesOf(store(), ptr(), query));
    }
    return Features::empty(store());
}

template<typename T>
Features FeatureBase<T>::parents() const
{
    return parents(nullptr);
}

template<typename T>
Features FeatureBase<T>::parents(const char* query) const
{
    if(isNode())
    {
        // only nodes can have both ways and relations as parents
        if (isAnonymousNode())
        {
            // anon nodes only have parent ways, and must have at least one
            return Features(View::parentWaysOf(store(),
                anonymousNode_.xy, query));
        }
        if (feature_.ptr.flags() && FeatureFlags::WAYNODE)
        {
            // feature node is part of at least one way
            if(feature_.ptr.isRelationMember())
            {
                // both ways and relations
                return Features(View::parentsOf(store(), ptr(), query));
            }
            // only ways
            return Features(View::parentWaysOf(store(), ptr(), query));
        }
        // fall through, feature node does not belong to a way,
        // but may be a relation member
    }
    if(feature_.ptr.isRelationMember())
    {
        return Features(View::parentRelationsOf(store(), ptr(), query));
    }
    return Features::empty(store());
}

} // namespace geodesk

// \endcond