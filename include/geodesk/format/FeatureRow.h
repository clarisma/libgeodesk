// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdint>
#include <clarisma/data/SmallArray.h>
#include <geodesk/feature/FeaturePtr.h>
#include <geodesk/format/KeySchema.h>
#include <geodesk/format/StringHolder.h>

namespace geodesk {

class FeatureRow
{
public:
    void populate(FeatureStore* store, FeaturePtr feature)
    {

    }

private:
    const KeySchema* keys_;
    clarisma::SmallArray<StringHolder,32> cols_;
};


} // namespace geodesk
