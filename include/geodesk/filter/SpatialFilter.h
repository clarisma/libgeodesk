// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/filter/Filter.h>
#include <geodesk/feature/NodePtr.h>
#include <geodesk/feature/RelationPtr.h>
#include <geodesk/feature/WayPtr.h>

namespace geodesk {

///
/// \cond lowlevel
///
class SpatialFilter : public Filter
{
public:
    SpatialFilter() :
        Filter(FilterFlags::USES_BBOX, FeatureTypes::ALL),
        bounds_(Box::ofWorld()) {}
    SpatialFilter(const Box& bounds) :
        Filter(FilterFlags::USES_BBOX, FeatureTypes::ALL),
        bounds_(bounds) {}
    SpatialFilter(int flags, FeatureTypes acceptedTypes, const Box& bounds) :
        Filter(flags, acceptedTypes),
        bounds_(bounds) {}

    const Box& bounds() const { return bounds_; }

protected:
    bool acceptFeature(FeatureStore* store, FeaturePtr feature) const;
    
    virtual bool acceptWay(WayPtr way) const { return false; }
    virtual bool acceptNode(NodePtr node) const { return false; }
    virtual bool acceptAreaRelation(FeatureStore* store, RelationPtr relation) const { return false; }
    virtual bool acceptMembers(FeatureStore* store, RelationPtr relation, RecursionGuard* guard) const;
    Box bounds_;
};

// \endcond
} // namespace geodesk
