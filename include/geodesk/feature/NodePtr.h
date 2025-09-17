// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/FeaturePtr.h>

namespace geodesk {

/// \cond lowlevel

class NodePtr : public FeaturePtr
{
public:
	NodePtr() {}
	explicit NodePtr(FeaturePtr f) : FeaturePtr(f.ptr()) { assert(isNull() || isNode()); }
	explicit NodePtr(DataPtr p) : FeaturePtr(p) {}
	// explicit NodePtr(const uint8_t* p) : FeaturePtr(p) {}

	int32_t x() const { return (p_-8).getInt(); }
	int32_t y() const { return (p_-4).getInt(); }
	Coordinate xy() const { return Coordinate(x(), y()); }
	Box bounds() const
	{
		return Box(x(), y(), x(), y());
	}

	bool intersects(const Box& bounds) const
	{
		return bounds.contains(x(), y());
	}

	/**
	 * Obtains a pointer to the node's relation table.
	 * The node must belong to a relation, otherwise the
	 * result is undefined.
	 */
	DataPtr relationTableFast() const
	{
		return (p_ + 12).follow();
	}

	bool hasSharedLocation() const
	{
		return flags() & FeatureFlags::SHARED_LOCATION;
	}

	/// Checks if this node is a *duplicate node*: it has no tags and
	/// has the same location as another node
	///
	bool isDuplicate() const
	{
		return (flags() & (FeatureFlags::SHARED_LOCATION | FeatureFlags::EXCEPTION_NODE))
			== (FeatureFlags::SHARED_LOCATION | FeatureFlags::EXCEPTION_NODE);
	}

	/// Checks if this node is an *orphan*: it has no tags and does not
	/// belong to a way or relation
	///
	bool isOrphan() const
	{
		return (flags() & (FeatureFlags::EXCEPTION_NODE |
			FeatureFlags::WAYNODE | FeatureFlags::RELATION_MEMBER))
			== FeatureFlags::EXCEPTION_NODE;
	}
};


struct SNode
{
	Coordinate xy;				// TODO: this is 4-byte aligned
								//  Coordinate may expect stricter alignment
	FeatureHeader header;
	int32_t tagTablePtr;		// tagged
	int32_t relTablePtr;		// can be omitted

	NodePtr ptr() const
	{
		return NodePtr(FeaturePtr(reinterpret_cast<const uint8_t*>(&header)));
	}
};

// \endcond
} // namespace geodesk
