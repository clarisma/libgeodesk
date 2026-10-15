// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/filter/SpatialFilter.h>
#include <geodesk/geom/Box.h>

namespace geodesk {

// TODO: Don't need to store the point, we already have the bbox
//  which *is* the point

class ContainsPointFilter : public SpatialFilter
{
public:
	ContainsPointFilter(Coordinate pt) : SpatialFilter(Box(pt)), point_(pt)	{}

	bool accept(FeatureStore* store, const FeaturePtr feature, FastFilterHint fast) const override;

private:
	Coordinate point_;
};

} // namespace geodesk
