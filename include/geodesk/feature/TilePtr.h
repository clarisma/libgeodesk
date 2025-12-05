// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/DataPtr.h>
#include <geodesk/feature/TileConstants.h>

/// \cond lowlevel

using clarisma::DataPtr;
using namespace TileConstants;

class TilePtr : public DataPtr
{
public:
	TilePtr() {}
	explicit TilePtr(const uint8_t* p) : DataPtr(p) {}
	explicit TilePtr(DataPtr p) : DataPtr(p) {}

	static uint32_t headerSize() { return 4; }

	uint32_t payloadSize() const
	{
		return getUnsignedInt();
	}

	uint32_t checksum() const
	{
		return (*this + payloadSize()).getUnsignedIntUnaligned();
	}

	uint32_t totalSize() const
	{
		return payloadSize() + headerSize();
	}

	/*
	DataPtr ptr() const { return p_; }
	DataPtr nodeIndex() const {	return p_ + NODE_INDEX_OFS; }
	DataPtr wayIndex() const { return p_ + WAY_INDEX_OFS; }
	DataPtr areaIndex() const { return p_ + AREA_INDEX_OFS; }
	DataPtr relationIndex() const { return p_ + RELATION_INDEX_OFS; }
	DataPtr exportTable() const { return p_ + EXPORTS_OFS; }

	explicit operator bool() const noexcept
	{
		return p_.ptr() != nullptr;
	}

	bool operator!() const noexcept
	{
		return !p_;
	}
	*/

    /*
    void query(FeatureTypes types, uint32_t indexBits,
        const Box& bounds, void (*accept)(FeaturePtr));
     */
};

/// \endcond lowlevel