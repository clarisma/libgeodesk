// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/filter/PreparedFilterFactory.h>
#include <geodesk/geom/geos/Geos.h>

namespace geodesk {

const Filter* PreparedFilterFactory::forFeature(FeatureStore* store, FeaturePtr feature)
{
	if (feature.isType(FeatureTypes::RELATIONS & FeatureTypes::AREAS))
	{
		RelationPtr relation(feature);
		bounds_ = relation.bounds();
		indexBuilder_.segmentizeAreaRelation(store, relation);
		return forPolygonal();
	}
	if (feature.isType(FeatureTypes::WAYS & FeatureTypes::AREAS))
	{
		WayPtr way(feature);
		bounds_ = way.bounds();
		indexBuilder_.segmentizeWay(way);
		return forPolygonal();
	}
	if (feature.isNode())
	{
		NodePtr node(feature);
		return forCoordinate(node.xy());
	}
	if (feature.isRelation())
	{
		RelationPtr relation(feature);
		RecursionGuard guard(relation);
		bounds_ = relation.bounds();
		indexBuilder_.segmentizeMembers(store, relation, guard);
		return forNonAreaRelation(store, relation);
	}
	assert(feature.isWay());
	WayPtr way(feature);
	bounds_ = way.bounds();
	indexBuilder_.segmentizeWay(way);
	return forLineal();
}

#ifdef GEODESK_WITH_GEOS
const Filter* PreparedFilterFactory::forGeometry(GEOSContextHandle_t context, GEOSGeometry* geom)
{
	int geomType = GEOSGeomTypeId_r(context, geom);
	unsigned int coordLen;
	const GEOSCoordSequence* seq;
	switch (geomType)
	{
	case GEOS_POINT:
		seq = GEOSGeom_getCoordSeq_r(context, geom);
		if (seq == NULL) return nullptr;
		GEOSCoordSeq_getSize_r(context, seq, &coordLen);
		if (coordLen == 0) return nullptr;
		double x, y;
		GEOSCoordSeq_getXY_r(context, seq, 0, &x, &y);
		return forCoordinate(Coordinate(x, y));

	case GEOS_LINESTRING:
	case GEOS_LINEARRING:
		seq = GEOSGeom_getCoordSeq_r(context, geom);
		if (seq == NULL) return nullptr;
		GEOSCoordSeq_getSize_r(context, seq, &coordLen);
		if (coordLen == 0) return nullptr;
		indexBuilder_.segmentizeCoords(context, seq);
		bounds_ = Geos::getEnvelope(context, geom);
		return forLineal();

	case GEOS_POLYGON:
		indexBuilder_.segmentizePolygon(context, geom);
		bounds_ = Geos::getEnvelope(context, geom);
		return forPolygonal();

	case GEOS_MULTIPOLYGON:
	{
		int count = GEOSGetNumGeometries_r(context, geom);
		for (int i = 0; i < count; i++)
		{
			const GEOSGeometry* child = GEOSGetGeometryN_r(context, geom, i);
			indexBuilder_.segmentizePolygon(context, child);
		}
		bounds_ = Geos::getEnvelope(context, geom);
		return forPolygonal();
	}

	default:
		return forGeometryCollection(context, geom);
	}
}
#endif


const Filter* PreparedFilterFactory::forBox(const Box& box)
{
	bounds_ = box;
	indexBuilder_.addLineSegment(box.topLeft(), box.topRight());
	indexBuilder_.addLineSegment(box.bottomRight(), box.topRight());
	indexBuilder_.addLineSegment(box.bottomLeft(), box.bottomRight());
	indexBuilder_.addLineSegment(box.bottomLeft(), box.topLeft());
	return forPolygonal();
}

} // namespace geodesk
