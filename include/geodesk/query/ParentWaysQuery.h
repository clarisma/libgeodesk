// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/query/Query.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/filter/FeatureNodeFilter.h>
#include <geodesk/filter/WayNodeFilter.h>

namespace geodesk {

class ParentWaysQuery
{
public:
    ParentWaysQuery(FeatureStore* store, Coordinate xy, NodePtr node) :
        bbox_(xy),
        featureNodeFilter_(node, nullptr),
        anonNodeFilter_(xy, nullptr),
        query_(store, bbox_,
            node.isNull() ?
                FeatureTypes::WAYS :
                (FeatureTypes::WAYS & FeatureTypes::WAYNODE_FLAGGED),
        store->borrowAllMatcher(),
        node.isNull() ?
            static_cast<Filter*>(&anonNodeFilter_) :
            static_cast<Filter*>(&featureNodeFilter_))
    {
    }

    WayPtr next() { return WayPtr(query_.next()); }

private:
    Box bbox_;
    FeatureNodeFilter featureNodeFilter_;
    WayNodeFilter anonNodeFilter_;
    Query query_;
};


} // namespace geodesk
