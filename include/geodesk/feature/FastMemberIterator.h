// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/RelatedIterator.h>
#include <geodesk/feature/RelationPtr.h>

namespace geodesk {

/// \cond lowlevel
/// An iterator that returns all members of a relation, without
/// any filtering and without retrieving roles.
///

class FastMemberIterator : public RelatedIteratorBase<FastMemberIterator,FeaturePtr,1,2>
{
public:
	FastMemberIterator(FeatureStore* store, RelationPtr relation) :
		RelatedIteratorBase(store, relation.bodyptr(), Tex::MEMBERS_START_TEX)
	{
	}

	bool readAndAcceptRole()    // CRTP override
	{
		if (member_ & MemberFlags::DIFFERENT_ROLE)
		{
			int rawRole = p_.getUnsignedShort();
			p_ += (rawRole & 1) ? 2 : 4;
		}
		return true;
	}
};



/*
class FastMemberIterator
{
public:
	FastMemberIterator(FeatureStore* store, RelationPtr relation);

	FeatureStore* store() const { return store_; }
	FeaturePtr next();

private:
	FeatureStore* store_;
	Tip currentTip_;
	int32_t currentMember_;
	DataPtr p_;
	DataPtr pForeignTile_;
};
*/

// \endcond


} // namespace geodesk
