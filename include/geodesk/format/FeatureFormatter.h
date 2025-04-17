// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "CoordinateFormat.h"
#include <clarisma/util/Buffer.h>
#include <geodesk/feature/FastMemberIterator.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/format/GeometryWriter.h>
#include <geodesk/geom/Coordinate.h>

namespace geodesk {

///
/// \cond lowlevel
///
template<typename Derived>
class FeatureFormatter : public CoordinateFormat
{
public:
	void writeGeometry(clarisma::Buffer& out, FeatureStore* store, FeaturePtr feature) const
    {
		if (feature.isWay())
		{
			self().writeGeometry(out, WayPtr(feature));
		}
		else if (feature.isNode())
		{
			self().writeGeometry(out, NodePtr(feature));
		}
		else
		{
			assert(feature.isRelation());
			self().writeGeometry(out, store, RelationPtr(feature));
		}
    }

	void writeGeometry(clarisma::Buffer& out, NodePtr node) const {}
	void writeGeometry(clarisma::Buffer& out, WayPtr way) const
    {
    	if(way.isArea())
        {
        	self().writeAreaGeometry(out, way);
        }
        else
        {
        	self().writeLinealGeometry(out, way);
        }
    }

	void writeGeometry(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const
	{
		if(rel.isArea())
		{
			self().writeAreaGeometry(out, store, rel);
		}
		else
		{
			self().writeCollectionGeometry(out, store, rel);
		}
	}

	void writeAreaGeometry(clarisma::Buffer& out, WayPtr way) const {}
	void writeLinealGeometry(clarisma::Buffer& out, WayPtr way) const {}
    void writeAreaGeometry(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const {}
    void writeCollectionGeometry(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const {}

	uint64_t writeMemberGeometries(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const
	{
		uint64_t count = 0;
		FastMemberIterator iter(store, rel);
		for (;;)
		{
			FeaturePtr member = iter.next();
			if (member.isNull()) break;
			out.writeByte(count > 0 ? ',' : coordGroupStartChar_);
			int memberType = member.typeCode();
			if (memberType == 1)
			{
				self().writeGeometry(out, WayPtr(member));
			}
			else if (memberType == 0)
			{
				self().writeGeometry(out, NodePtr(member));
			}
			else
			{
				assert(memberType == 2);
				self().writeGeometry(out, store, RelationPtr(member));
			}
			count++;
		}
		if (count) writeByte(coordGroupEndChar_);
		return count;
	}

protected:
	Derived& self() noexcept
	{
		return static_cast<Derived&>(*this);
	}

	const Derived& self() const noexcept
	{
		return static_cast<const Derived&>(*this);
	}
};

// \endcond

} // namespace geodesk
