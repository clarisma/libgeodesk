// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/filter/ContainsPointFilter.h>
#include <geodesk/geom/polygon/RobustPointInPolygon.h>

namespace geodesk {

bool ContainsPointFilter::accept(FeatureStore* store, FeaturePtr feature, FastFilterHint /* ignored */) const
{
	if (feature.isArea())
	{
		if (feature.isWay())
		{
			// TODO: Does it make sense to perform the bbox shortcut test?
			//  The query bbox for a point should already have taken care
			//  of this (but filter combos may result in larger bbox)

			return RobustPointInPolygon::classifyBoundaryWay(
				WayPtr(feature), point_) != RobustPointInPolygon::OUTSIDE;
				// point is also considered within if on boundary
		}
		assert(feature.isRelation());
		return RobustPointInPolygon::classifyAreaRelation(
			store, RelationPtr(feature), point_) != RobustPointInPolygon::OUTSIDE;
			// point is also considered within if on boundary
	}
	if (feature.isWay())
	{
		return RobustPointInPolygon::classifyBoundaryWay(
			WayPtr(feature), point_) == RobustPointInPolygon::BOUNDARY;
			// A linear way contains a point that lies on its boundary
	}
	if (feature.isNode())
	{
		// A node contains another node only if their coordinates are the same
		NodePtr node(feature);
		return node.xy() == point_;
	}
	// TODO: non-area relations
	return false;
}

} // namespace geodesk
