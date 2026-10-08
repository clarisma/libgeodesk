// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef GEODESK_WITH_GEOS

#include <geodesk/geom/GeometryBuilder.h>
#include <geodesk/geom/polygon/Polygonizer.h>
#include <geodesk/geom/polygon/Ring.h>

namespace geodesk {

geos::geom::Geometry::Ptr GeometryBuilder::buildWayGeometry(const FeaturePtr way, geos::geom::GeometryFactory& geosContext)
{
	WayCoordinateIterator iter;
	int areaFlag = way.flags() & FeatureFlags::AREA;
	iter.start(way, areaFlag);
	int count = iter.storedCoordinatesRemaining() + (areaFlag ? 1 : 0);
	geos::geom::CoordinateSequence coordSeq = geos::geom::CoordinateSequence::XY(count);  // 2D points
	for (int i=0; i<count; i++)
	{
		Coordinate c = iter.next();
		coordSeq.setAt(geos::geom::CoordinateXY(c.x, c.y), i);
	}
	if (areaFlag)
	{
		return geosContext.createPolygon(geosContext.createLinearRing(std::move(coordSeq)));
	}
	else
	{
		return geosContext.createLineString(coordSeq);
	}
}

// TODO: consolidate with buildPointGeometry
geos::geom::Geometry::Ptr GeometryBuilder::buildNodeGeometry(const NodePtr node, geos::geom::GeometryFactory& geosContext)
{
	geos::geom::CoordinateSequence coordSeq = geos::geom::CoordinateSequence::XY(1);
    coordSeq.setAt(geos::geom::CoordinateXY(node.x(), node.y()), 0);
    return geosContext.createPoint(std::move(coordSeq));
}

geos::geom::Geometry::Ptr GeometryBuilder::buildPointGeometry(int32_t x, int32_t y, geos::geom::GeometryFactory& geosContext)
{
	geos::geom::CoordinateSequence coordSeq = geos::geom::CoordinateSequence::XY(1);
    coordSeq.setAt(geos::geom::CoordinateXY(x, y), 0);
    return geosContext.createPoint(std::move(coordSeq));
}


geos::geom::Geometry::Ptr GeometryBuilder::buildBoxGeometry(const Box& box, geos::geom::GeometryFactory& geosContext)
{
	geos::geom::CoordinateSequence coordSeq = geos::geom::CoordinateSequence::XY(5);  // 5 points, 2D
	coordSeq.setAt(geos::geom::CoordinateXY(box.minX(), box.minY()), 0);
	coordSeq.setAt(geos::geom::CoordinateXY(box.minX(), box.maxY()), 1);
	coordSeq.setAt(geos::geom::CoordinateXY(box.maxX(), box.maxY()), 2);
	coordSeq.setAt(geos::geom::CoordinateXY(box.maxX(), box.minY()), 3);
	coordSeq.setAt(geos::geom::CoordinateXY(box.minX(), box.minY()), 4);
	return geosContext.createPolygon(geosContext.createLinearRing(coordSeq));
}


geos::geom::Geometry::Ptr GeometryBuilder::buildAreaRelationGeometry(FeatureStore* store, RelationPtr relation, geos::geom::GeometryFactory& geosContext)
{
	Polygonizer polygonizer;
	polygonizer.createRings(store, relation);
	polygonizer.assignAndMergeHoles();
	return polygonizer.createPolygonal(geosContext);
}


geos::geom::Geometry::Ptr GeometryBuilder::buildRelationGeometry(FeatureStore* store, RelationPtr relation, geos::geom::GeometryFactory& geosContext)
{
	if (relation.isArea())
	{
		return buildAreaRelationGeometry(store, relation, geosContext);
	}
	RelationGeometryBuilder rgb(store, relation, geosContext);
	return rgb.build();
}


RelationGeometryBuilder::RelationGeometryBuilder(FeatureStore* store, 
	RelationPtr relation, geos::geom::GeometryFactory& geosContext) :
	store_(store),
	context_(geosContext),
	guard_(relation)
{
	gatherMembers(relation);
}


void RelationGeometryBuilder::gatherMembers(RelationPtr relation)
{
	FastMemberIterator iter(store_, relation);
	for (;;)
	{
		FeaturePtr member = iter.next();
		if (member.isNull()) break;
		int memberType = member.typeCode();
		geos::geom::Geometry::Ptr g;
		if (memberType == 1)
		{
			WayPtr memberWay(member);
			if (memberWay.isPlaceholder()) continue;
			g = GeometryBuilder::buildWayGeometry(memberWay, context_);
		}
		else if (memberType == 0)
		{
			NodePtr memberNode(member);
			if (memberNode.isPlaceholder()) continue;
			g = GeometryBuilder::buildNodeGeometry(memberNode, context_);
		}
		else
		{
			assert(memberType == 2);
			RelationPtr childRel(member);
			if (childRel.isPlaceholder() || !guard_.checkAndAdd(childRel)) continue;
			if (childRel.isArea())
			{
				g = GeometryBuilder::buildAreaRelationGeometry(store_, childRel, context_);
			}
			else
			{
				gatherMembers(childRel);
				continue;
			}
		}
		geoms_.push_back(std::move(g));
	}
}


geos::geom::Geometry::Ptr RelationGeometryBuilder::build()
{
	// TODO: different collection types

	return context_.createGeometryCollection(std::move(geoms_));
}


geos::geom::Geometry::Ptr GeometryBuilder::buildFeatureGeometry(FeatureStore* store, FeaturePtr feature, geos::geom::GeometryFactory& geosContext)
{
	int typeCode = feature.typeCode();
	if (typeCode == 1)
	{
		return buildWayGeometry(WayPtr(feature), geosContext);
	}
	if (typeCode == 0)
	{
		return buildNodeGeometry(NodePtr(feature), geosContext);
	}
	assert(feature.isRelation());
	return buildRelationGeometry(store, RelationPtr(feature), geosContext);
}

} // namespace geodesk

#endif // GEODESK_WITH_GEOS