// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <geodesk/feature/RelationPtr.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/feature/WayCoordinateIterator.h>
#include <geodesk/geom/polygon/Polygonizer.h>
#include <geodesk/geom/polygon/Ring.h>
#include <geodesk/geom/polygon/RingCoordinateIterator.h>
#include "WkbType.h"
#include <vector>
#include <cassert>

namespace geodesk {

///
/// \cond lowlevel
///
template<typename Derived>
class WkbWriterBase
{
	using enum WkbType;

public:
	void writeFeature(FeatureStore* store, FeaturePtr feature)
	{
		if (feature.isWay())
		{
			writeWay(WayPtr(feature));
		}
		else if (feature.isNode())
		{
			writeNode(NodePtr(feature));
		}
		else
		{
			assert(feature.isRelation());
			writeRelation(store, RelationPtr(feature));
		}
	}

	void writePoint(Coordinate pt)
	{
		writeHeader(POINT);
		self().writeCoordinate(pt);
	}

	void writeNode(NodePtr node)
	{
		writeHeader(POINT);
		self().writeCoordinate(node.xy());
	}

	void writeWay(WayPtr way)
	{
		bool isArea = way.isArea();
		writeHeader(isArea ? POLYGON : LINESTRING);
		if (isArea) self().writeUnsignedInt(1);
		writeWayCoordinates(way);
	}

	void writeAreaWay(WayPtr way)
	{
		writeHeader(POLYGON);
		self().writeUnsignedInt(1);
		writeWayCoordinates(way);
	}

	void writeWayCoordinates(WayPtr way)
	{
		WayCoordinateIterator iter(way);
		self().writeUnsignedInt(iter.coordinatesRemaining());
		for (;;)
		{
			Coordinate c = iter.next();
			if (c.isNull()) break;
			self().writeCoordinate(c);
		}
	}

	void writeRelation(FeatureStore* store, RelationPtr rel)
	{
		if (rel.isArea())
		{
			writeAreaRelation(store, rel);
		}
		else
		{
			writeCollectionRelation(store, rel);
		}
	}

	void writeAreaRelation(FeatureStore* store, RelationPtr rel)
	{
		Polygonizer polygonizer;
		polygonizer.createRings(store, rel);
		polygonizer.assignAndMergeHoles();
		const Polygonizer::Ring* ring = polygonizer.outerRings();
		int count = ring ? ring->number() : 0;
		if (count == 0)	[[unlikely]]
		{
			writeHeader(POLYGON);
			self().writeUnsignedInt(count);
		}
		else if (count == 1)	[[likely]]
		{
			writePolygon(ring);
		}
		else
		{
			writeHeader(MULTIPOLYGON);
			self().writeUnsignedInt(count);
			do
			{
				writePolygon(ring);
				ring = ring->next();
			}
			while (ring);
		}
	}

	void writePolygon(const Polygonizer::Ring* outer)
	{
		assert(outer);
		writeHeader(POLYGON);
		const Polygonizer::Ring* inner = outer->firstInner();
		int holeCount = inner ? inner->number() : 0;
		self().writeUnsignedInt(holeCount + 1);
		writeRing(outer);
		while (inner)
		{
			writeRing(inner);
			inner = inner->next();
		}
	}

	void writeRing(const Polygonizer::Ring* ring)
	{
		RingCoordinateIterator iter(ring);
		self().writeUnsignedInt(iter.coordinatesRemaining());
		for (;;)
		{
			Coordinate c = iter.next();
			if (c.isNull()) break;
			self().writeCoordinate(c);
		}
	}

	void writeCollectionRelation(FeatureStore* store, RelationPtr rel)
	{
		std::vector<FeaturePtr> tempMembers;
		WkbType type = GEOMETRY;
		FastMemberIterator iter(store, rel);
		for (;;)
		{
			FeaturePtr member = iter.next();
			if (member.isNull()) break;
			if (type != GEOMETRYCOLLECTION)
			{
				WkbType newType;
				if (member.isWay())
				{
					newType = member.isArea() ? MULTIPOLYGON : MULTILINESTRING;
				}
				else if (member.isNode())
				{
					newType = MULTIPOINT;
				}
				else
				{
					assert(member.isRelation());
					newType = GEOMETRYCOLLECTION;
				}
				type = type==GEOMETRY ? newType : type;
				type = type==newType ? type : GEOMETRYCOLLECTION;
			}
			tempMembers.push_back(member);
		}
		writeHeader(tempMembers.size() == 0 ? GEOMETRYCOLLECTION : type);
		self().writeUnsignedInt(tempMembers.size());
		for (FeaturePtr member : tempMembers)
		{
			writeFeature(store, member);
		}
	}

	Derived& self() noexcept
	{
		return static_cast<Derived&>(*this);
	}

	const Derived& self() const noexcept
	{
		return static_cast<const Derived&>(*this);
	}

private:
	void writeHeader(WkbType type)
	{
		self().writeByte(1);	// little endian
		self().writeUnsignedInt(static_cast<uint32_t>(type));
	}

	// === CRTP virtual methods ===

	void writeCoordinate(Coordinate c) // CRTP virtual
	{
		self().writeDouble(c.lon());
		self().writeDouble(c.lat());
	}

	void writeByte(uint8_t v); // CRTP virtual
	void writeUnsignedInt(uint32_t v); // CRTP virtual
	void writeDouble(double v); // CRTP virtual
};

// \endcond
} // namespace geodesk
