// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/RelatedIterator.h>
#include <geodesk/feature/GlobalStrings.h>
#include <geodesk/feature/RelationPtr.h>
#include <geodesk/feature/WayPtr.h>

namespace geodesk {

/// \cond lowlevel
/// An iterator that only returns member ways of a relation with the
/// role "outer" or "inner"
///

class AreaMemberIterator : public RelatedIteratorBase<AreaMemberIterator,WayPtr,1,2>
{
public:
	AreaMemberIterator(FeatureStore* store, RelationPtr relation) :
		RelatedIteratorBase(store, relation.bodyptr(), Tex::MEMBERS_START_TEX),
		rawCurrentRole_(0)	// technically 1 (global string "")
	{
	}

	bool readAndAcceptRole()    // CRTP override
	{
		if (member_ & MemberFlags::DIFFERENT_ROLE)
		{
			rawCurrentRole_ = p_.getUnsignedShort();
			p_ += (rawCurrentRole_ & 1) ? 2 : 4;
		}
		return rawCurrentRole_ == (((GlobalStrings::OUTER) << 1) | 1) ||
			rawCurrentRole_ == (((GlobalStrings::INNER) << 1) | 1);
    }

	bool accept(FeaturePtr feature)    // CRTP override
	{
    	return feature.isWay();
	}

protected:
    int rawCurrentRole_;
};

// \endcond


} // namespace geodesk
