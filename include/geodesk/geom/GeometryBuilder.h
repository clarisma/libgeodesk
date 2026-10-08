// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#ifdef GEODESK_WITH_GEOS

#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/CoordinateSequence.h>
#include <geodesk/feature/RelationPtr.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/geom/Box.h>

namespace geodesk {

/// \cond lowlevel

class GeometryBuilder
{
public:
	static geos::geom::Geometry::Ptr buildFeatureGeometry(FeatureStore* store, FeaturePtr feature, geos::geom::GeometryFactory& geosContext);
	static geos::geom::Geometry::Ptr buildNodeGeometry(NodePtr node, geos::geom::GeometryFactory& geosContext);
	static geos::geom::Geometry::Ptr buildWayGeometry(FeaturePtr way, geos::geom::GeometryFactory& geosContext);
	static geos::geom::Geometry::Ptr buildAreaRelationGeometry(FeatureStore* store, RelationPtr relation, geos::geom::GeometryFactory& geosContext);
	static geos::geom::Geometry::Ptr buildRelationGeometry(FeatureStore *store, RelationPtr relation, geos::geom::GeometryFactory& geosContext);
	static geos::geom::Geometry::Ptr buildPointGeometry(int32_t x, int32_t y, geos::geom::GeometryFactory& geosContext);
	static geos::geom::Geometry::Ptr buildBoxGeometry(const Box& box, geos::geom::GeometryFactory& geosContext);
};


class RelationGeometryBuilder
{
public:
	RelationGeometryBuilder(FeatureStore* store, RelationPtr relation, geos::geom::GeometryFactory& geosContext);
	geos::geom::Geometry::Ptr build();

private:
	void gatherMembers(RelationPtr relation);

	FeatureStore* store_;
	geos::geom::GeometryFactory& context_;
	RecursionGuard guard_;
	std::vector< geos::geom::Geometry::Ptr> geoms_;
};

// \endcond
} // namespace geodesk

#endif // GEODESK_WITH_GEOS