// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#ifdef GEODESK_WITH_GEOS

#include <geos_c.h>
#include <geodesk/feature/RelationPtr.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/geom/Box.h>

namespace geodesk {

/// \cond lowlevel

class GeometryBuilder
{
public:
	static GEOSGeometry* buildFeatureGeometry(const FeatureStore* store, FeaturePtr feature, GEOSContextHandle_t geosContext);
	static GEOSGeometry* buildNodeGeometry(NodePtr node, GEOSContextHandle_t geosContext);
	static GEOSGeometry* buildWayGeometry(FeaturePtr way, GEOSContextHandle_t geosContext);
	static GEOSGeometry* buildAreaRelationGeometry(const FeatureStore* store, RelationPtr relation, GEOSContextHandle_t geosContext);
	static GEOSGeometry* buildRelationGeometry(const FeatureStore *store, RelationPtr relation, GEOSContextHandle_t geosContext);
	static GEOSGeometry* buildPointGeometry(int32_t x, int32_t y, GEOSContextHandle_t geosContext);
	static GEOSGeometry* buildBoxGeometry(const Box& box, GEOSContextHandle_t geosContext);
};


class RelationGeometryBuilder
{
public:
	RelationGeometryBuilder(const FeatureStore* store, RelationPtr relation, GEOSContextHandle_t geosContext);
	GEOSGeometry* build();

private:
	void gatherMembers(RelationPtr relation);

	const FeatureStore* store_;
	GEOSContextHandle_t context_;
	RecursionGuard guard_;
	std::vector< GEOSGeometry*> geoms_;
};

// \endcond
} // namespace geodesk

#endif // GEODESK_WITH_GEOS