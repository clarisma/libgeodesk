// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#ifdef GEODESK_WITH_GEOS
#include <geos_c.h>
#endif
#include <geodesk/feature/RelationPtr.h>
#include <geodesk/geom/index/MCIndexBuilder.h>

namespace geodesk {


class PreparedFilterFactory
{
public:
	const Filter* forFeature(FeatureStore* store, FeaturePtr feature);
	#ifdef GEODESK_WITH_GEOS
	const Filter* forGeometry(GEOSContextHandle_t geosContext, const GEOSGeometry* geom);
	#endif
	const Filter* forBox(const Box& box);
	virtual const Filter* forCoordinate(Coordinate point) { return nullptr; };

	const Box& bounds() const { return bounds_; }
	MCIndex buildIndex() { return indexBuilder_.build(bounds_); }

protected:
	virtual const Filter* forPolygonal() { return nullptr; };
	virtual const Filter* forLineal() { return nullptr; };
	virtual const Filter* forPuntal() { return nullptr; };

	// TODO: Does this require parameters?
	// By default, the PreparedFilterFactory indexes all ways of a relation
	// But for filters like "intersects" to work, we need *two* indexes:
	// one for areas and one for lineal features
	virtual const Filter* forNonAreaRelation(FeatureStore* store, RelationPtr feature)
	{
		return nullptr;
	};
	#ifdef GEODESK_WITH_GEOS
	virtual const Filter* forGeometryCollection(GEOSContextHandle_t geosContext, const GEOSGeometry* geom)
	{
		return nullptr;
	};
	#endif

private:
	Box bounds_;
	MCIndexBuilder indexBuilder_;
};

} // namespace geodesk
