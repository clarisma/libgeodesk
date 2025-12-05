// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#ifdef GEODESK_WITH_OGR

#include <ogr_geometry.h>
#include <geodesk/feature/RelationPtr.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/geom/Box.h>
#include <geodesk/geom/polygon/Ring.h>

namespace geodesk {

/// \cond lowlevel

class OgrGeometryBuilder
{
public:
	static OGRGeometry* buildFeatureGeometry(FeatureStore* store, FeaturePtr feature);
	static OGRPoint* buildPoint(Coordinate c)
	{
		return new OGRPoint(c.lon(), c.lat());
	}

	static OGRPoint* buildPoint(NodePtr node)
	{
		return buildPoint(node.xy());
	}

	static OGRLineString* buildLineString(WayPtr way);
	static OGRGeometry* buildWayGeometry(WayPtr way);
	static OGRGeometry* buildAreaRelationGeometry(FeatureStore* store, RelationPtr relation);
	static OGRGeometry* buildRelationGeometry(FeatureStore *store, RelationPtr relation);

private:
	static void setPoints(OGRLineString* line, WayPtr way);
};


class OgrRelationGeometryBuilder
{
public:
	OgrRelationGeometryBuilder(FeatureStore* store, RelationPtr relation);
	OGRGeometry* build();

private:
	void gatherMembers(RelationPtr relation);

	FeatureStore* store_;
	std::vector<OGRGeometry*> geoms_;
};

// \endcond
} // namespace geodesk

#endif // GEODESK_WITH_OGR