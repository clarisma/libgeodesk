// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/RelatedIterator.h>
#include <geodesk/feature/RelationPtr.h>

namespace geodesk {

/// \cond lowlevel
///
class ParentRelationIterator : public RelatedIterator<ParentRelationIterator,RelationPtr,0,2>
{
public:
    ParentRelationIterator(FeatureStore* store, DataPtr pRelTable,
        const MatcherHolder* matcher, const Filter* filter) :
        RelatedIterator(store, pRelTable, Tex::RELATIONS_START_TEX,
            matcher, filter)
    {
    }

    ParentRelationIterator(FeatureStore* store, DataPtr pRelTable) :
        RelatedIterator(store, pRelTable, Tex::RELATIONS_START_TEX,
            store->borrowAllMatcher(), nullptr)
    {
    }
};

// \endcond
} // namespace geodesk
