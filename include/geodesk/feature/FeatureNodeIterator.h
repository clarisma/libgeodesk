// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/RelatedIterator.h>
#include <geodesk/feature/WayPtr.h>

namespace geodesk {

/// \cond lowlevel
///
class FeatureNodeIterator : public RelatedIterator<FeatureNodeIterator,NodePtr,0,-2>
{
public:
    FeatureNodeIterator(FeatureStore* store, DataPtr pBody,
        int flags, const MatcherHolder* matcher, const Filter* filter) :
        RelatedIterator(store, pBody - (flags & FeatureFlags::RELATION_MEMBER) - 2,
            Tex::WAYNODES_START_TEX, matcher, filter)
    {
        member_ = (flags & FeatureFlags::WAYNODE) ? 0 : MemberFlags::LAST;
    }
};

/*
class FeatureNodeIterator
{
public:
    explicit FeatureNodeIterator(FeatureStore* store);
    FeatureStore* store() const { return store_; }
    void start(DataPtr pBody, int flags, const MatcherHolder* matcher, const Filter* filter);
    NodePtr next();

private:
    FeatureStore* store_;
    const MatcherHolder* matcher_;
    const Filter* filter_;
    Tip currentTip_;
    int32_t currentNode_;
    DataPtr p_;
    DataPtr pForeignTile_;
};
*/

// \endcond
} // namespace geodesk
