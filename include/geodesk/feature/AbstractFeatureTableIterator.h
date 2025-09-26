// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/DataPtr.h>
#include <geodesk/feature/types.h>
#include <geodesk/feature/Tex.h>

namespace geodesk {

// TODO

/// \cond lowlevel

template<int ExtraFlags, int Step>
class AbstractFeatureTableIterator
{
public:
	AbstractFeatureTableIterator(int_fast32_t handle, clarisma::DataPtr pTable) :
		pTile_(pTable - handle),
		ofs_(handle),
		currentOfs_(0),
		member_(0),
		tipDelta_(0)
	{
	}

	bool isForeign() const { return member_ & MemberFlags::FOREIGN; }
	bool isInDifferentTile() const
	{ 
		assert(isForeign());
		return member_ & (1 << (2 + ExtraFlags));
	}

	TipDelta tipDelta() const { return TipDelta(tipDelta_); }
	TexDelta texDelta() const
	{
		return member_ >> (4 + ExtraFlags);
	}

	int_fast32_t localHandle() const
	{
		return currentOfs_ + (static_cast<int_fast32_t>(member_) >> 1);
	}

	DataPtr ptr() const { return pTile_ + ofs_; }
	int_fast32_t currentOfs() const { return currentOfs_;  }
	DataPtr currentPtr() const { return pTile_ + currentOfs_; }

protected:
	bool isLast() const
	{
		return member_ & MemberFlags::LAST;
	}

  	void fetchNext()
    {
		currentOfs_ = ofs_;
		uint16_t lowerWord = (pTile_ + ofs_).getUnsignedShort();
		if (lowerWord & MemberFlags::FOREIGN)
        {
            if(lowerWord & (1 << (3 + ExtraFlags)))  [[unlikely]]
            {
            	// wide TEX delta
				ofs_ += Step;
            	uint16_t upperWord = (pTile_ + ofs_).getUnsignedShort();
                member_ = (static_cast<int32_t>(upperWord) << 16) | lowerWord;
            }
            else
            {
            	member_ = static_cast<int16_t>(lowerWord);
            }
            if(isInDifferentTile())
            {
            	// foreign member in different tile
            	ofs_ += Step;
            	tipDelta_ = (pTile_ + ofs_).getShort();
            	if (tipDelta_ & 1)
            	{
            		// wide TIP delta
            		ofs_ += Step;
            		tipDelta_ = (tipDelta_ & 0xffff) |
						(static_cast<int32_t>((pTile_ + ofs_).getShort()) << 16);
            	}
            	tipDelta_ >>= 1;     // signed
            }
		}
		else
        {
        	ofs_ += Step;
        	uint16_t upperWord = (pTile_ + ofs_).getUnsignedShort();
        	member_ = (static_cast<int32_t>(upperWord) << 16) | lowerWord;
        }
    }

	clarisma::DataPtr pTile_;
	int_fast32_t ofs_;
	int_fast32_t currentOfs_;
	int32_t member_;			// must be signed for locals
	int32_t tipDelta_;
};

/// \endcond
} // namespace geodesk
