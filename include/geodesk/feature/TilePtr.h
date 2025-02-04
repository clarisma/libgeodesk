// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/store/BlobPtr.h>
#include <geodesk/feature/TileConstants.h>

using clarisma::DataPtr;
using namespace TileConstants;

class TilePtr : public clarisma::BlobPtr
{
public:
	explicit TilePtr(BlobPtr p) : BlobPtr(p) {}

	DataPtr nodeIndex() const {	return p_ + NODE_INDEX_OFS; }
	DataPtr wayIndex() const { return p_ + WAY_INDEX_OFS; }
	DataPtr areaIndex() const { return p_ + AREA_INDEX_OFS; }
	DataPtr relationIndex() const { return p_ + RELATION_INDEX_OFS; }
	DataPtr exportTable() const { return p_ + EXPORTS_OFS; }

    /*
    void query(FeatureTypes types, uint32_t indexBits,
        const Box& bounds, void (*accept)(FeaturePtr));
     */
};
