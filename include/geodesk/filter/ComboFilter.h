// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/filter/SpatialFilter.h>

namespace geodesk {

class RoleFilter;

///
/// \cond lowlevel
///
class ComboFilter : public SpatialFilter
{
public:
    ComboFilter(const Filter* a, const Filter* b);
    ~ComboFilter();

    bool accept(FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const override;
    int acceptTile(Tile tile) const override;
    const RoleFilter* roleFilter() const;
    const Filter* withoutRoleFilter() const;

private:
    ComboFilter(const ComboFilter& other, std::vector<const Filter*>&& filters) :
        SpatialFilter(other),
        filters_(std::move(filters)) {}

    void add(const Filter* f);

    std::vector<const Filter*> filters_;
};

// \endcond
} // namespace geodesk
