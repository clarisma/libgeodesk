// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/RelatedIterator.h>
#include <geodesk/feature/WayPtr.h>

namespace geodesk {

/// \cond lowlevel
///
class   FeatureNodeIterator : public RelatedIterator<FeatureNodeIterator,NodePtr,0,-2>
{
public:
    FeatureNodeIterator(FeatureStore* store, DataPtr pBody,
        int flags, const MatcherHolder* matcher, const Filter* filter) :
        RelatedIterator(store, pBody - (flags & FeatureFlags::RELATION_MEMBER) - 2,
            Tex::WAYNODES_START_TEX, matcher, filter)
    {
        member_ = (flags & FeatureFlags::WAYNODE) ? 0 : MemberFlags::LAST;
    }

    FeatureNodeIterator(FeatureStore* store, WayPtr way,
        const MatcherHolder* matcher, const Filter* filter = nullptr) :
        FeatureNodeIterator(store, way.bodyptr(), way.flags(), matcher, filter)
    {
    }

    FeatureNodeIterator(FeatureStore* store, WayPtr way) :
        FeatureNodeIterator(store, way, store->borrowAllMatcher())
    {
    }
};


// \endcond
} // namespace geodesk
