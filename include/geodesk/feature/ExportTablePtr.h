// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/DataPtr.h>
#include <geodesk/feature/FeaturePtr.h>
#include <geodesk/feature/Tex.h>
#include <geodesk/feature/TileConstants.h>

namespace geodesk {

/// \cond lowlevel

using clarisma::DataPtr;
using namespace TileConstants;

class ExportTablePtr : public DataPtr
{
public:
	ExportTablePtr() {}
	explicit ExportTablePtr(const uint8_t* p) : DataPtr(p) {}
	explicit ExportTablePtr(DataPtr p) : DataPtr(p) {}

	uint32_t count() const { return getUnsignedInt(); }
	FeaturePtr featureAt(Tex tex) const
	{
		uint32_t slot = static_cast<uint32_t>(tex);
		assert(slot < count());
		return FeaturePtr((*this + slot * 4).follow());
	}
};

/// \endcond lowlevel
} // namespace geodesk