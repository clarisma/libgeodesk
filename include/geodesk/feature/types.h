// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#ifdef GEODESK_PYTHON
#include <Python.h>
#endif
#include <cstdint>
#include <geodesk/feature/FeatureType.h>
#include <geodesk/feature/Tip.h>

namespace geodesk {

/// \cond lowlevel

enum FeatureIndexType
{
    NODES = 0,
    WAYS = 1,
    AREAS = 2,
    RELATIONS = 3
};

class IndexBits
{
public:
    static uint32_t fromCategory(int category)
    {
        return category == 0 ? 0 : (1 << (category - 1));
    }
};

namespace FeatureFlags
{
    enum
    {
        LAST_SPATIAL_ITEM = 1,
        AREA = 1 << 1,
        RELATION_MEMBER = 1 << 2,
        WAYNODE = 1 << 5,
        MULTITILE_WEST = 1 << 6,
        MULTITILE_NORTH = 1 << 7,
        SHARED_LOCATION = 1 << 8,
        EXCEPTION_NODE = 1 << 9,
        UNMODIFIED = 1 << 10,
        DELETED = 1 << 11
    };
}


enum MemberFlags
{
    LAST = 1,
    FOREIGN = 2,
    DIFFERENT_ROLE = 4,
    DIFFERENT_TILE = 8,
        // TODO: This will change in 2.0 for relation tables and feature-node tables
        //  (moves to Bit 2 == value 4, to accommodate more TEX bits)
    WIDE_NODE_TEX = 8,
    WIDE_RELATION_TEX = 8,
    WIDE_MEMBER_TEX = 16,
};

namespace FeatureConstants
{
    static const Tip START_TIP(0x4000);     
        // TODO: move to Tip? No, not really a characteristic of TIP,
        //  it is driven by the encoding used by member/node/relation tables
    static const int MAX_COMMON_KEY = (1 << 13) - 2;
    static const int MAX_COMMON_ROLE = (1 << 15) - 1;
};

// \endcond
} // namespace geodesk
