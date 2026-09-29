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
const Filter* PreparedFilterFactory::forGeometry(geos::geom::GeometryFactory& context, const geos::geom::Geometry& geom)
{
	geos::geom::GeometryTypeId geomType = geom.getGeometryTypeId();
	switch (geomType)
	{
	case geos::geom::GeometryTypeId::GEOS_POINT: {
		auto& pt = dynamic_cast<const geos::geom::Point&>(geom);
		if (pt.isEmpty()) return nullptr;
		return forCoordinate(Coordinate(pt.getX(), pt.getY()));
	}
	case geos::geom::GeometryTypeId::GEOS_LINESTRING:
	case geos::geom::GeometryTypeId::GEOS_LINEARRING: {
		const geos::geom::CoordinateSequence::Ptr seq = geom.getCoordinates();
		unsigned int coordLen = seq->getSize();
		if (coordLen == 0) return nullptr;
		indexBuilder_.segmentizeCoords(context, *seq);
		bounds_ = Geos::getEnvelope(context, geom);
		return forLineal();
	}
	case geos::geom::GeometryTypeId::GEOS_POLYGON: {
		indexBuilder_.segmentizePolygon(context, dynamic_cast<const geos::geom::Polygon&>(geom));
		bounds_ = Geos::getEnvelope(context, geom);
		return forPolygonal();
	}
	case geos::geom::GeometryTypeId::GEOS_MULTIPOLYGON:
	{
		auto& multiPoly = dynamic_cast<const geos::geom::MultiPolygon&>(geom);
		int count = multiPoly.getNumGeometries();
		for (int i = 0; i < count; i++)
		{
			const geos::geom::Polygon& child = *multiPoly.getGeometryN(i);
			indexBuilder_.segmentizePolygon(context, child);
		}
		bounds_ = Geos::getEnvelope(context, multiPoly);
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
