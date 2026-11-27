// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/data/HashSet.h>
#include <geodesk/filter/SpatialFilter.h>

namespace geodesk {

class ConnectedFilter : public SpatialFilter
{
public:
	ConnectedFilter(const FeatureStore* store, FeaturePtr feature);

	bool accept(const FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const override;

protected:
	bool acceptWay(WayPtr way) const override;
	bool acceptNode(NodePtr node) const override;
	bool acceptAreaRelation(const FeatureStore* store, RelationPtr relation) const override;

private:
	void collectWayPoints(WayPtr way);
	void collectMemberPoints(const FeatureStore* store, RelationPtr relation, RecursionGuard& guard);

	uint64_t self_;
	clarisma::HashSet<Coordinate> points_;
};

} // namespace geodesk
