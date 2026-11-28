// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/varint.h>
#include <geodesk/feature/NodePtr.h>

namespace geodesk {

/// \cond lowlevel

class WayPtr : public FeaturePtr
{
public:
	WayPtr() {}
	WayPtr(const WayPtr& other) = default;
	explicit WayPtr(FeaturePtr f) : FeaturePtr(f.ptr()) { assert(isNull() || isWay()); }
	explicit WayPtr(clarisma::DataPtr p) : FeaturePtr(p) {}

	bool hasFeatureNodes() const noexcept
	{
		return flags() & FeatureFlags::WAYNODE;
	}

	uint32_t nodeCount() const noexcept
	{
		const uint8_t* p = bodyptr();
		uint32_t rawCount = clarisma::readVarint32(p);
		return rawCount + isArea();
	}

	bool contains(const FeatureStore* store, NodePtr node) const noexcept;
	bool containsAnonNode(Coordinate xy) const noexcept;

	/// @brief Based on the given pointer to the way's encoded
	/// coordinates and the raw (stored) node count, returns
	/// a pointer to the encoded waynode IDs.
	///
	/// Important: The GOL must have been built with optional
	/// waynode ID storage enabled, or the returned pointer
	/// will be invalid.
	///
	static const uint8_t* wayNodeIDs(const uint8_t* pCoords, int rawNodeCount) noexcept
	{
		clarisma::skipVarints(pCoords, rawNodeCount * 2);
		return pCoords;
	}
};

// \endcond



} // namespace geodesk
