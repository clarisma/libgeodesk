// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/AbstractFeatureTableIterator.h>
#include <geodesk/feature/FeaturePtr.h>

namespace geodesk {

/// \cond lowlevel
///
class MemberTableIterator : public AbstractFeatureTableIterator<1,2>
{
public:
	MemberTableIterator(int_fast32_t handle, DataPtr pTable) :
		AbstractFeatureTableIterator(handle, pTable),
		currentRoleOfs_(0),
		rawRole_(1)
	{
	}

	bool next()
	{
		if (isLast()) return false;
		fetchNext();
		ofs_ += 2;
		if (member_ & MemberFlags::DIFFERENT_ROLE)
		{
			// TODO: could read without branching if over-reading is allowed
			currentRoleOfs_ = ofs_;
			rawRole_ = (pTile_ + ofs_).getUnsignedShort();
			if (rawRole_ & 1)
			{
				ofs_ += 2;
			}
			else
			{
				rawRole_ = (pTile_ + ofs_).getIntUnaligned();
				ofs_ += 4;
			}
		}
		return true;
	}

	bool hasDifferentRole() const { return member_ & MemberFlags::DIFFERENT_ROLE; }
	bool hasGlobalRole() const { return rawRole_ & 1; }
	bool hasLocalRole() const { return (rawRole_ & 1) == 0; }
	int32_t globalRoleFast() const
	{
		assert(hasGlobalRole());
		return rawRole_ >> 1;
	}
	int_fast32_t localRoleHandleFast() const
	{
		assert(hasLocalRole());
		// LOG("local string role = #%d", currentRoleOfs_ + (rawRole_ >> 1));
		return currentRoleOfs_ + (rawRole_ >> 1);
	}
	DataPtr localRoleStringFast() const
	{
		return pTile_ + localRoleHandleFast();
	}
	
	int_fast32_t localHandle() const	// shadows
	{
		// This differs from base implementation; we need to mask
		// bits in addition to the shift

        /*
        int_fast32_t adjustedOffset = (currentOfs_ & 0xffff'fffc);
        int_fast32_t adjustedMember =
            (static_cast<int_fast32_t>(member_ & 0xffff'fff8) >> 1);
        printf("adjustedOffset = %d, adjustedMember = %d",
            adjustedOffset, adjustedMember);
        */

		return (currentOfs_ & 0xffff'fffc) + 
			((static_cast<int_fast32_t>(member_) &
                static_cast<int32_t>(0xffff'fff8)) >> 1);
	}

	FeaturePtr localMember() const
	{
		return FeaturePtr(pTile_ + localHandle());
	}

protected:
	int_fast32_t currentRoleOfs_;
	int32_t rawRole_;
};

// \endcond


} // namespace geodesk
