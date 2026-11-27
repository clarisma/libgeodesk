// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/filter/SpatialFilter.h>
#include <geodesk/geom/Mercator.h>
#include <geodesk/match/Matcher.h>

namespace geodesk {

/**
 * Accepts all features that are within the given distance from the given point.
 */
class PointDistanceFilter : public SpatialFilter
{
public:
	PointDistanceFilter(double meters, Coordinate point);

	bool accept(const FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const override;

private:
	bool segmentsWithinDistance(WayPtr way, int areaFlag) const;
	bool isWithinDistance(WayPtr way) const;
	bool isAreaWithinDistance(const FeatureStore* store, RelationPtr relation) const;
	bool areMembersWithinDistance(const FeatureStore* store, RelationPtr relation, RecursionGuard& guard) const;

	Coordinate point_;
	double distanceSquared_;
};

} // namespace geodesk
