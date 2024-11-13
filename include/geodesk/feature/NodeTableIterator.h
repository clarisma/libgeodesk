// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/AbstractFeatureTableIterator.h>
#include <geodesk/feature/NodePtr.h>

namespace geodesk {

/// \cond lowlevel
///
class NodeTableIterator : public AbstractFeatureTableIterator<0,-2>
{
public:
	NodeTableIterator(int_fast32_t handle, DataPtr pTable) :
		AbstractFeatureTableIterator(handle, pTable)
	{
	}

	NodePtr localNode() const
	{
		return NodePtr(pTile_ + localHandle());
	}

	bool next()
	{
		if(isLast()) return false;
		ofs_ -= 2;
		fetchNext();
		return true;
	}
};

// \endcond

} // namespace geodesk
