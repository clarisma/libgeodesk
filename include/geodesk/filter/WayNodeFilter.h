// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/filter/Filter.h>

namespace geodesk {

// Important: Ensure that this class does not require cleanup,
// see FeatureIteratorBase::~FeatureIteratorBase()

class WayNodeFilter : public Filter
{
public:
    // We don't need to set acceptedTypes because this Filter
    // will never be combined with others; it is only used for
    // finding ways that contain a specific node
    WayNodeFilter(Coordinate coord, const Filter* filter) :
        coord_(coord),
        secondaryFilter_(filter)
    {
    }

    bool accept(FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const override;

private:
    Coordinate coord_;
    const Filter* secondaryFilter_;
};

} // namespace geodesk