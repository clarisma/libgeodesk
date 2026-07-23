// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/filter/PointDistanceFilter.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/feature/FastMemberIterator.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/geom/polygon/RobustPointInPolygon.h>
#include <geodesk/geom/Distance.h>

namespace geodesk {

// TODO: Conditionally accelerate if distance is far enough that
// some tiles could lie outside the circle

PointDistanceFilter::PointDistanceFilter(double meters, Coordinate point)
	: point_(point)
{
	double d = Mercator::unitsFromMeters(meters, point.y);
	bounds_ = Box::unitsAroundXY((int32_t)std::ceil(d), point);
	distanceSquared_ = d * d;
}


bool PointDistanceFilter::segmentsWithinDistance(WayPtr way, int areaFlag) const
{
    WayCoordinateIterator iter;
    iter.start(way, areaFlag);
    Coordinate c = iter.next();
    double x1 = c.x;
    double y1 = c.y;
    for(;;)
    {
        c = iter.next();
        if (c.isNull()) break;
        double x2 = c.x;
        double y2 = c.y;
        if (Distance::pointSegmentSquared(x1, y1, x2, y2,
            point_.x, point_.y) < distanceSquared_)
        {
            return true;
        }
        x1 = x2;
        y1 = y2;
    }
    return false;
}


bool PointDistanceFilter::isWithinDistance(WayPtr way) const
{
    if (way.isArea())
    {
        if (segmentsWithinDistance(way, FeatureFlags::AREA)) return true;

        // The distance of a point that lies within a polygon is zero;
        // we need to perform p-in-p check because the edges themselves
        // may be far away from the comparison point

        Box bounds = way.bounds();
        if (!bounds.contains(point_)) return false;

        return RobustPointInPolygon::classifyBoundaryWay(way, point_) !=
            RobustPointInPolygon::OUTSIDE;
    }
    return segmentsWithinDistance(way, 0);
}


bool PointDistanceFilter::isAreaWithinDistance(FeatureStore* store, RelationPtr relation) const
{
    // measure distance to the ways that define shell and holes, and
    // also perform point in polygon test
    int loc = 0;

    FastMemberIterator iter(store, relation);
    for (;;)
    {
        FeaturePtr member = iter.next();
        if (member.isNull()) break;
        if (!member.isWay()) continue;
        WayPtr memberWay(member);
        if (memberWay.isPlaceholder()) continue;
        int memberFlags = member.flags();
        if (segmentsWithinDistance(memberWay, memberFlags)) return true;
        if (RobustPointInPolygon::mustClassifyBoundaryWay(memberWay, point_))
        {
            int memberLoc = RobustPointInPolygon::classifyBoundaryWay(memberWay, point_);
            loc = RobustPointInPolygon::combineResult(loc, memberLoc);
        }
    }
    return loc != RobustPointInPolygon::OUTSIDE;
        // INSIDE or BOUNDARY
        // TODO: It can't be BOUNDARY, because we'd get a distance of 0,
        //  so may be just check for INSIDE (which is 1, i.e. true)
}


bool PointDistanceFilter::accept(FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const
{
    FeatureType type = feature.type();
    if (type == FeatureType::WAY)
    {
        WayPtr way(feature);
        return isWithinDistance(way);
    }
    if (type == FeatureType::NODE)
    {
        NodePtr node(feature);
        return Distance::pointsSquared(node.x(), node.y(), 
            point_.x, point_.y) < distanceSquared_;
    }
    assert(type == FeatureType::RELATION);
    if (feature.isArea())
    {
        return isAreaWithinDistance(store, RelationPtr(feature));
    }
    RelationPtr relation(feature);
    RecursionGuard guard(relation);
    return areMembersWithinDistance(store, relation, guard);
}


bool PointDistanceFilter::areMembersWithinDistance(FeatureStore* store, RelationPtr relation, RecursionGuard& guard) const
{
    FastMemberIterator iter(store, relation);
    for (;;)
    {
        FeaturePtr member = iter.next();
        if (member.isNull()) break;
        int typeCode = member.typeCode();
        if (typeCode == 1)
        {
            WayPtr memberWay(member);
            if (!memberWay.isPlaceholder() && isWithinDistance(memberWay)) return true;
        }
        else if (typeCode == 0)
        {
            NodePtr memberNode(member);
            if (!memberNode.isPlaceholder() && Distance::pointsSquared(
                memberNode.x(), memberNode.y(), point_.x, point_.y) < distanceSquared_)
            {
                return true;
            }
        }
        else
        {
            assert(member.isRelation());
            RelationPtr memberRel(member);
            if (!memberRel.isPlaceholder() && guard.checkAndAdd(memberRel) &&
                areMembersWithinDistance(store, memberRel, guard))
            {
                return true;
            }
        }
    }
    return false;
}

} // namespace geodesk
