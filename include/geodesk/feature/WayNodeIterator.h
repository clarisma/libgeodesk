// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <geodesk/feature/FeatureNodeIterator.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/feature/WayCoordinateIterator.h>

#include "ForeignFeatureRef.h"

namespace geodesk {

///
/// \cond lowlevel
///
class WayNodeIterator 
{
public:
    WayNodeIterator(FeatureStore* store, WayPtr way, bool duplicateLast, bool wayNodeIDs) :
        nodes_(store, way.bodyptr(), way.flags(), store->borrowAllMatcher(), nullptr),
        pID_(nullptr),
        id_(0),
        firstId_(0)
    {
        coords_.start(way, duplicateLast ? way.flags() : 0);
        if(wayNodeIDs)
        {
            pID_ = coords_.wayNodeIDs();
            id_ = clarisma::readSignedVarint64(pID_);
            firstId_ = id_;
        }
        fetchNextNode();
    }

    bool next()
    {
        Coordinate xy = coords_.next();
        if(xy.isNull()) return false;
        node_ = NodePtr();
        ref_ = ForeignFeatureRef();
        if(pID_)
        {
            if(coords_.storedCoordinatesRemaining() > 0)
            {
                id_ += clarisma::readSignedVarint64(pID_);
            }
            else
            {
                id_ = firstId_;
            }
        }
        if(!nextNode_.isNull())
        {
            if(nextNode_.xy() == xy)
            {
                assert(id_ == 0 || id_ == nextNode_.id());
                node_ = nextNode_;
                id_ = node_.id();
                fetchNextNode();
            }
        }
        return true;
    }

    NodePtr node() const noexcept { return node_; }
    Coordinate xy() const noexcept { return coords_.current(); }
    int64_t id() const noexcept { return id_; }
    ForeignFeatureRef foreignRef() const noexcept { return ref_; }

private:
    void fetchNextNode()
    {
        nextNode_ = nodes_.next();
        if(nodes_.isForeign())
        {
            nextRef_ = ForeignFeatureRef(nodes_.tip(), nodes_.tex());
        }
    }

    FeatureNodeIterator nodes_;
    WayCoordinateIterator coords_;
    const uint8_t* pID_;
    NodePtr nextNode_;
    NodePtr node_;
    ForeignFeatureRef nextRef_;
    ForeignFeatureRef ref_;
    int64_t id_;
    int64_t firstId_;
};

// \endcond
} // namespace geodesk