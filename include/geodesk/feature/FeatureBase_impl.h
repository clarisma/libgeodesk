// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/FeatureBase.h>
#include <geodesk/feature/Features.h>
#include <geodesk/feature/Nodes.h>
#ifdef GEODESK_WITH_GEOS
#include <geodesk/geom/GeometryBuilder.h>
#endif
#ifdef GEODESK_WITH_OGR
#include <geodesk/geom/OgrGeometryBuilder.h>
#endif

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
    FeatureTypes types = 0;
    if(isNode())
    {
        // only nodes can have both ways and relations as parents
        if (isAnonymousNode())
        {
            // anon nodes only have parent ways, and must have at least one
            return Features(View::parentWaysOf(store(),
                anonymousNode_.xy, query));
        }

        // feature node is part of at least one way
        types = (feature_.ptr.flags() & FeatureFlags::WAYNODE) ?
            (FeatureTypes::WAYS & FeatureTypes::WAYNODE_FLAGGED) : 0;

        // fall through, feature node does not belong to a way,
        // but may be a relation member
    }
    types |= feature_.ptr.isRelationMember() ? FeatureTypes::RELATIONS : 0;
    if (types)
    {
        return Features(View::parentsOf(store(), ptr(), types, query));
    }
    return Features::empty(store());
}

#ifdef GEODESK_WITH_GEOS
template<typename T>
GEOSGeometry* FeatureBase<T>::toGeometry(GEOSContextHandle_t geosContext) const
{
    if (isWay())
    {
        return GeometryBuilder::buildWayGeometry(feature_.ptr, geosContext);
    }
    if (isNode())
    {
        if (isAnonymousNode())
        {
            return GeometryBuilder::buildPointGeometry(
                anonymousNode_.xy.x, anonymousNode_.xy.y, geosContext);
        }
        return GeometryBuilder::buildNodeGeometry(
            NodePtr(feature_.ptr), geosContext);
    }
    return GeometryBuilder::buildRelationGeometry(store_,
        RelationPtr(feature_.ptr), geosContext);
}
#endif

#ifdef GEODESK_WITH_OGR
template<typename T>
OGRGeometry* FeatureBase<T>::toOgrGeometry() const
{
    if (isWay())
    {
        return OgrGeometryBuilder::buildWayGeometry(feature_.ptr);
    }
    if (isNode())
    {
        return OgrGeometryBuilder::buildPoint(
            isAnonymousNode() ? anonymousNode_.xy :
            NodePtr(feature_.ptr).xy());
    }
    return OgrGeometryBuilder::buildRelationGeometry(store_, feature_.ptr);
}
#endif

} // namespace geodesk

// \endcond