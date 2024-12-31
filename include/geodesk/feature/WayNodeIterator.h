// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/cli/Console.h>
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
    struct WayNode
    {
        NodePtr feature;
        Coordinate xy;
        int64_t id;
        ForeignFeatureRef foreign;
    };

    WayNodeIterator(FeatureStore* store, WayPtr way, bool duplicateFirst, bool wayNodeIDs) :
        nodes_(store, way.bodyptr(), way.flags(), store->borrowAllMatcher(), nullptr),
        pID_(nullptr),
        id_(0),
        firstId_(0)
    {
        coords_.start(way, duplicateFirst ? way.flags() : 0);
        if(wayNodeIDs)
        {
            pID_ = coords_.wayNodeIDs();
            id_ = clarisma::readSignedVarint64(pID_);
            firstId_ = duplicateFirst ? id_ : 0;
        }
        fetchNextNode();
    }

    WayNode next()
    {
        Coordinate xy = coords_.next();
        NodePtr node = NodePtr();
        ForeignFeatureRef foreign = ForeignFeatureRef();
        int64_t id = id_;
        if(pID_)
        {
            if(coords_.storedCoordinatesRemaining() > 0)
            {
                id_ += clarisma::readSignedVarint64(pID_);
            }
            else
            {
                id_ = firstId_;
                firstId_ = 0;
            }
        }
        if(!nextNode_.isNull())
        {
            if(nextNode_.xy() == xy)
            {
                if(id != 0 && id != nextNode_.id())
                {
                    clarisma::Console::log("  Mismatch node/%lld vs node/%lld",
                        id, nextNode_.id());
                }
                assert(id == 0 || id == nextNode_.id());
                node = nextNode_;
                foreign = nextRef_;
                id = node.id();
                fetchNextNode();
            }
        }

        /*
        if(id == 1430960416)
        {
            printf("!!!");
        }
        */

        return {node,xy,id,foreign};
    }

private:
    void fetchNextNode()
    {
        nextNode_ = nodes_.next();
        if(nodes_.isForeign())
        {
            nextRef_ = ForeignFeatureRef(nodes_.tip(), nodes_.tex());
        }
        else
        {
            nextRef_ = ForeignFeatureRef();
        }
    }

    FeatureNodeIterator nodes_;
    WayCoordinateIterator coords_;
    const uint8_t* pID_;
    NodePtr nextNode_;
    ForeignFeatureRef nextRef_;
    int64_t id_;
    int64_t firstId_;
};

// \endcond
} // namespace geodesk