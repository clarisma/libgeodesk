// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/AbstractFeatureTableIterator.h>
#include <geodesk/feature/RelationPtr.h>

namespace geodesk {

/// \cond lowlevel
///
class RelationTableIterator : public AbstractFeatureTableIterator<0,2>
{
public:
    RelationTableIterator(int_fast32_t handle, RelationTablePtr rels) :
        AbstractFeatureTableIterator(handle, rels.ptr())
    {
    }

    RelationPtr localRelation() const
    {
        return RelationPtr(pTile_ + localHandle());
    }

    bool next()
    {
        if(isLast()) return false;
        fetchNext();
        ofs_ += 2;
        return true;
    }
};

// \endcond
} // namespace geodesk
