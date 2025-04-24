// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "CoordinateFormat.h"
#include <clarisma/util/Buffer.h>
#include <geodesk/feature/FastMemberIterator.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/format/KeySchema.h>
#include <geodesk/geom/Coordinate.h>
#include <geodesk/format/FilteredTagWalker.h>

namespace geodesk {

///
/// \cond lowlevel
///
template<typename Derived>
class FeatureFormatter : public CoordinateFormat
{
public:
	FeatureFormatter(KeySchema* schema = nullptr) : schema_(schema) {}

	void writeFeatureGeometry(clarisma::Buffer& out, FeatureStore* store, FeaturePtr feature) const
    {
		if (feature.isWay())
		{
			self().writeWayGeometry(out, WayPtr(feature));
		}
		else if (feature.isNode())
		{
			self().writeNodeGeometry(out, NodePtr(feature));
		}
		else
		{
			assert(feature.isRelation());
			self().writeRelationGeometry(out, store, RelationPtr(feature));
		}
    }

	void writeNodeGeometry(clarisma::Buffer& out, NodePtr node) const {}
	void writeWayGeometry(clarisma::Buffer& out, WayPtr way) const
    {
    	if(way.isArea())
        {
        	self().writeAreaWayGeometry(out, way);
        }
        else
        {
        	self().writeLinealWayGeometry(out, way);
        }
    }

	void writeRelationGeometry(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const
	{
		if(rel.isArea())
		{
			self().writeAreaRelationGeometry(out, store, rel);
		}
		else
		{
			self().writeCollectionRelationGeometry(out, store, rel);
		}
	}

	void writeAreaWayGeometry(clarisma::Buffer& out, WayPtr way) const {}
	void writeLinealWayGeometry(clarisma::Buffer& out, WayPtr way) const {}
    void writeAreaRelationGeometry(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const {}
    void writeCollectionRelationGeometry(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const {}

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
				self().writeWayGeometry(out, WayPtr(member));
			}
			else if (memberType == 0)
			{
				self().writeNodeGeometry(out, NodePtr(member));
			}
			else
			{
				assert(memberType == 2);
				self().writeRelationGeometry(out, store, RelationPtr(member));
			}
			count++;
		}
		if (count) out.writeByte(coordGroupEndChar_);
		return count;
	}

    void writeTagValue(clarisma::Buffer& out, const TagWalker& iter) const
    {
		if (iter.isStringValue()) [[likely]] // string
		{
			if (iter.isWideValue())	[[unlikely]]
			{
				self().writeStringTagValue(out, iter.localStringValueFast());
			}
			else
			{
				self().writeStringTagValue(out, iter.globalStringValueFast());
			}
		}
		else
		{
			self().writeNumberTagValue(out, iter.numberValueFast());
		}
	}

	void writeNumberTagValue(clarisma::Buffer& out, clarisma::Decimal d) const
    {
    	out << d;
	}

	void writeDefaultId(clarisma::Buffer& out, FeaturePtr feature)
	{
		out.writeByte("NWRS"[feature.typeCode()]);
		out << feature.id();
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

    const KeySchema* schema_;
};

// \endcond

} // namespace geodesk
