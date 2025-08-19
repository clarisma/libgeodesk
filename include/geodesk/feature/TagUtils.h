// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/TagTablePtr.h>

#include "FeatureStore.h"
#include "Tag.h"
#include "TagWalker.h"
#include "geodesk/format/KeySchema.h"

namespace geodesk {

/// \cond lowlevel

class TagUtils
{
public:
    template<typename Container>
    static void getTags(
        FeatureStore* store, TagTablePtr tags,
        const KeySchema& keys, Container& results)
    {
        TagWalker tw(tags, store->strings());
        while (tw.next())
        {
            int col;
            if(tw.keyCode() >= 0) [[likely]]
            {
                col = keys.columnOfGlobal(tw.keyCode());
            }
            else
            {
                col = keys.columnOfLocal(tw.key()->toStringView());
            }
            if (col > 0)
            {
                results.emplace_back(tw.key(), tw.value());
            }
        }
    }
};

// \endcond
} // namespace geodesk
