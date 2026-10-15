// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once
#include <geodesk/query/TileScannerBase.h>

namespace geodesk {

class FeatureFinder : public TileScannerBase<FeatureFinder>
{
public:
    FeaturePtr find(TilePtr tilePtr, TypedFeatureId typedId, Box bounds)
    {
        typedId_ = typedId;
        bounds_ = bounds;
        result_ = FeaturePtr();
        scanFeatures(tilePtr);
        return result_;
    }

private:
    friend class TileScannerBase<FeatureFinder>;

    bool acceptNodes() const  // CRTP override
    {
        return typedId_.isNode();
    }

    bool acceptLinearWays() const  // CRTP override
    {
        return typedId_.isWay();
    }

    bool acceptAreas() const  // CRTP override
    {
        return !typedId_.isNode();
    }

    bool acceptNonAreaRelations() const  // CRTP override
    {
        return typedId_.isRelation();
    }

    bool acceptBranch(const Box& bbox) const // CRTP override
    {
        return bbox.intersects(bounds_);
    }

    bool node(NodePtr node)   // CRTP override
    {
        if (node.typedId() == typedId_)
        {
            result_ = node;
            return true;
        }
        return false;
    }

    bool wayOrRelation(FeaturePtr feature)   // CRTP override
    {
        if (feature.typedId() == typedId_)
        {
            result_ = feature;
            return true;
        }
        return false;
    }

    TypedFeatureId typedId_ = TypedFeatureId(0);
    Box bounds_;
    FeaturePtr result_;
};

} // namespace geodesk