// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/DataPtr.h>
#include <geodesk/feature/FeatureHeader.h>
#include <geodesk/feature/FeatureTypes.h>
#include <geodesk/feature/TagTablePtr.h>
#include <geodesk/geom/Box.h>

namespace geodesk {

class FeatureStore;
class Matcher;
class MatcherHolder;
class Filter;
class StringTable;

/// Pointer to a Feature's data.
///
/// This class if part of the **Low-Level API**. It is not intended to
/// be used directly by applications.
///
class GEODESK_API FeaturePtr
{
public:
	FeaturePtr() {}
	FeaturePtr(const uint8_t* p) : p_(p) {}
	FeaturePtr(const FeaturePtr& other) : p_(other.p_) {}

	operator DataPtr () const noexcept { return p_; }

	uint64_t id() const noexcept
	{
		uint32_t hi = p_.getUnsignedInt() >> 8;
		uint32_t lo = (p_ + 4).getUnsignedInt();
		return (static_cast<uint64_t>(hi) << 32) | lo;
	}

	uint64_t typedId() const noexcept
	{
		return (id() << 2) | typeCode();
	}

	FeatureHeader header()
	{
		return FeatureHeader(p_.getUnsignedLongUnaligned());
	}

	// TODO: should just get pointer
	Box bounds() const noexcept
	{
		assert(!isNode());
		return Box((p_-16).getInt(), (p_-12).getInt(), (p_-8).getInt(), (p_-4).getInt());
	}

	bool isNull() const noexcept { return !p_; }
	int  flags() const noexcept	{ return p_.getInt();	}
	bool isArea() const	noexcept { return p_.getUnsignedInt() & FeatureFlags::AREA; }
	bool isNode() const	noexcept { return type() == FeatureType::NODE; }
	bool isWay() const noexcept { return type() == FeatureType::WAY; }
	bool isRelation() const	noexcept { return type() == FeatureType::RELATION; }
	bool isRelationMember() const noexcept { return flags() & FeatureFlags::RELATION_MEMBER; }
	bool isType(FeatureTypes types) const noexcept { return types.acceptFlags(flags()); }
	bool intersects(const Box& bounds) const noexcept
	{
		assert(!isNode());
		return (!((p_-16).getInt() > bounds.maxX() ||
			(p_-12).getInt() > bounds.maxY() ||
			(p_-8).getInt() < bounds.minX() ||
			(p_-4).getInt() < bounds.minY()));
	}

	TagTablePtr tags() const noexcept
	{
		return TagTablePtr::readFrom(p_ + 8);
	}

	DataPtr ptr() const noexcept { return p_; }
	DataPtr bodyptr() const noexcept { return (p_ + 12).follow(); }

	/**
	 * Obtains a pointer to the feature's relation table.
	 * The feature must belong to a relation, otherwise the
	 * result is undefined.
	 */
	DataPtr relationTableFast() const
	{
		assert(isRelationMember());
		// For nodes, the body pointer points to the reltable
		// For ways and relations, the reltable pointer sits right
		// before the body anchor
		DataPtr ppRelTable = isNode() ? (p_ + 12) : (bodyptr() - 4);
		return ppRelTable.followUnaligned();
			// TODO: unaligned only for ways/relations
	}

	int32_t relationTableHandleFast(const uint8_t* base) const
	{
		assert(isRelationMember());
		// For nodes, the body pointer points to the reltable
		// For ways and relations, the reltable pointer sits right
		// before the body anchor
        DataPtr ppRelTable = isNode() ? (p_ + 12) : (bodyptr() - 4);
		return static_cast<int32_t>((ppRelTable - base) +
			ppRelTable.getIntUnaligned());
	}

	/**
	 * A 64-bit value that uniquely identifies the feature based on its
	 * type and ID.
	 * 
	 * TODO: Changes in 2.0
	 */
	int64_t idBits() const
	{
		return p_.getLong() & 0xffff'ffff'ffff'ff18;
	}

	int64_t hash() const
	{
		return idBits();
	}

	FeatureType type() const
	{
		return static_cast<FeatureType>((p_.getUnsignedInt() >> 3) & 3);
	}

	int typeCode() const
	{
		return (p_.getUnsignedInt() >> 3) & 3;
	}

	// TODO: not valid for NodeRef
	int32_t minX() const { return (p_-16).getInt(); }
	int32_t minY() const { return (p_-12).getInt(); }
	int32_t maxX() const { return (p_-8).getInt(); }
	int32_t maxY() const { return (p_-4).getInt(); }

	const char* typeName() const;
	std::string toString() const;

	bool operator<(const FeaturePtr& other) const noexcept
	{
		if(typeCode() != other.typeCode())
		{
			return typeCode() < other.typeCode();
		}
		return id() < other.id();
	}

protected:
	DataPtr p_;
};

/// \cond lowlevel

struct SFeature
{
	Box bounds;
	FeatureHeader header;
	int32_t tagTablePtr;		// tagged
	int32_t relTablePtr;

	FeaturePtr ptr() const
	{
		return FeaturePtr(reinterpret_cast<const uint8_t*>(&header));
	}
};

// \endcond


} // namespace geodesk
