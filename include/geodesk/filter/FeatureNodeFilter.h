// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/filter/Filter.h>
#include <geodesk/feature/NodePtr.h>

namespace geodesk {

// Important: Ensure that this class does not require cleanup,
// see FeatureIteratorBase::~FeatureIteratorBase()

class FeatureNodeFilter : public Filter
{
public:
    // We don't need to set acceptedTypes because this Filter
    // will never be combined with others; it is only used for
    // finding ways that contain a specific node
    FeatureNodeFilter(NodePtr node, const Filter* filter) :
        node_(node),
        secondaryFilter_(filter)
    {
    }

    NodePtr node() const { return node_; }
    const Filter* secondaryFilter() const { return secondaryFilter_; }

    bool accept(FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const override;

private:
    NodePtr node_;
    const Filter* secondaryFilter_;
};

} // geodesk
