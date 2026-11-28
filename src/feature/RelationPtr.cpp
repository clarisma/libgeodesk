// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/RelationPtr.h>
#include <geodesk/feature/NodePtr.h>
#include <geodesk/feature/ParentRelationIterator.h>

namespace geodesk {

bool RelationPtr::contains(const FeatureStore* store, FeaturePtr member) const
{
    if (!member.isRelationMember()) return false;
    if (member.isNode())
    {
        if (!bounds().contains(NodePtr(member).xy())) return false;
    }
    else
    {
        if (!bounds().contains(member.bounds())) return false;
    }

    ParentRelationIterator iter(store, member.relationTableFast());
    for (;;)
    {
        RelationPtr r = iter.next();
        if (r.isNull()) return false;
        if (r.ptr() == ptr()) return true;
    }
}

} // namespace geodesk
