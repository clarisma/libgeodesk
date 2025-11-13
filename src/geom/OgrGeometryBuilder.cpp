// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef GEODESK_WITH_OGR

#include <geodesk/geom/OgrGeometryBuilder.h>
#include <geodesk/geom/polygon/Polygonizer.h>
#include <geodesk/geom/polygon/Ring.h>

namespace geodesk {

void OgrGeometryBuilder::setPoints(OGRLineString* line, WayPtr way)
{
	WayCoordinateIterator iter;
	int areaFlag = way.flags() & FeatureFlags::AREA;
	iter.start(way, areaFlag);
	int count = iter.storedCoordinatesRemaining() + (areaFlag ? 1 : 0);
	line->setNumPoints(count, FALSE);
	for (int i=0; i<count; i++)
	{
		Coordinate c = iter.next();
		line->setPoint(i, c.lon(), c.lat());
	}
}

OGRLineString* OgrGeometryBuilder::buildLineString(WayPtr way)
{
	OGRLineString* line = new OGRLineString();
	setPoints(line, WayPtr(way));
	return line;
}

OGRGeometry* OgrGeometryBuilder::buildWayGeometry(const WayPtr way)
{
	if (!way.isArea()) return buildLineString(way);
	OGRPolygon* polygon = new OGRPolygon();
	OGRLinearRing* ring = new OGRLinearRing();
	setPoints(ring, way);
	polygon->addRingDirectly(ring);
	return polygon;
}


OGRGeometry* OgrGeometryBuilder::buildAreaRelationGeometry(FeatureStore* store, RelationPtr relation)
{
	Polygonizer polygonizer;
	polygonizer.createRings(store, relation);
	polygonizer.assignAndMergeHoles();
	return polygonizer.createOgrPolygonal();
}


OGRGeometry* OgrGeometryBuilder::buildRelationGeometry(FeatureStore* store, RelationPtr relation)
{
	if (relation.isArea())
	{
		return buildAreaRelationGeometry(store, relation);
	}
	OgrRelationGeometryBuilder rgb(store, relation);
	return rgb.build();
}


OgrRelationGeometryBuilder::OgrRelationGeometryBuilder(
	FeatureStore* store, RelationPtr relation) :
	store_(store)
{
	gatherMembers(relation);
}


void OgrRelationGeometryBuilder::gatherMembers(RelationPtr relation)
{
	FastMemberIterator iter(store_, relation);
	for (;;)
	{
		FeaturePtr member = iter.next();
		if (member.isNull()) break;
		int memberType = member.typeCode();
		OGRGeometry* g;
		if (memberType == 1)
		{
			WayPtr memberWay(member);
			g = OgrGeometryBuilder::buildWayGeometry(memberWay);
		}
		else if (memberType == 0)
		{
			NodePtr memberNode(member);
			g = OgrGeometryBuilder::buildPoint(memberNode);
		}
		else
		{
			assert(memberType == 2);
			RelationPtr childRel(member);
			if (childRel.isArea())
			{
				g = OgrGeometryBuilder::buildAreaRelationGeometry(store_, childRel);
			}
			else
			{
				gatherMembers(childRel);
				continue;
			}
		}
		geoms_.push_back(g);
	}
}


OGRGeometry* OgrRelationGeometryBuilder::build()
{
	// TODO: different collection types
	OGRGeometryCollection* coll = new OGRGeometryCollection();
	for (OGRGeometry* g : geoms_)
	{
		coll->addGeometryDirectly(g);
	}
	return coll;
}


OGRGeometry* OgrGeometryBuilder::buildFeatureGeometry(FeatureStore* store, FeaturePtr feature)
{
	int typeCode = feature.typeCode();
	if (typeCode == 1)
	{
		return buildWayGeometry(WayPtr(feature));
	}
	if (typeCode == 0)
	{
		return buildPoint(NodePtr(feature));
	}
	assert(feature.isRelation());
	return buildRelationGeometry(store, RelationPtr(feature));
}

} // namespace geodesk

#endif // GEODESK_WITH_OGR