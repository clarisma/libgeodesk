// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/FeaturePtr.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/feature/Tex.h>
#include <geodesk/feature/TileConstants.h>

namespace geodesk {

/// \cond lowlevel
///

// TODO: Use std::conditional_t
//  e.g.
//   using MatcherType = std::conditional_t<Filtered, const MatcherHolder*, std::nullptr_t>;
//   using FilterType = std::conditional_t<Filtered, const Filter*, std::nullptr_t>;

template<typename Derived, int ExtraFlags, int Step>
class RelatedIteratorBase
{
public:
    RelatedIteratorBase(FeatureStore* store, DataPtr p, Tex initialTex) :
        store_(store),
        currentTex_(initialTex),
        member_(0),
        p_(p)
    {
    }

protected:
    Derived* self() const noexcept { return static_Cast<Derived*>(this); }

    bool isForeign() const { return member_ & MemberFlags::FOREIGN; }
    bool isInDifferentTile() const
    {
        assert(isForeign());
        return member_ & (1 << (2 + ExtraFlags));
    }

    FeaturePtr fetchNext()
    {
        uint16_t lowerWord = p_.getUnsignedShort();
        if (lowerWord & MemberFlags::FOREIGN)
        {
            if(lowerWord & (1 << (3 + ExtraFlags)))  [[unlikely]]
            {
                // wide TEX delta
                p_ += Step;
                uint16_t upperWord = p_.getUnsignedShort();
                member_ = (static_cast<int32_t>(upperWord) << 16) | lowerWord;
            }
            else
            {
                member_ = static_cast<int16_t>(lowerWord);
            }
            if(isInDifferentTile())
            {
                // foreign member in different tile
                p_ += Step;
                int32_t tipDelta = p_.getShort();
                if (tipDelta & 1)
                {
                    // wide TIP delta
                    p_ += Step;
                    tipDelta = (tipDelta & 0xffff) |
                        (static_cast<int32_t>(p_.getShort()) << 16);
                }
                tipDelta >>= 1;     // signed
                currentTip_ += tipDelta;
                DataPtr pTile = store_->fetchTile(currentTip_);
                    // TODO: Deal with missing tiles
                pExports_ = (pTile + TileConstants::EXPORTS_OFS).follow();
            }
            currentTex_ += member_ >> (4 + ExtraFlags);
            return FeaturePtr((pExports_ + currentTex_ * 4).follow());
        }
        else
        {
            p_ += Step;
            uint16_t upperWord = p_.getUnsignedShort();
            member_ = (static_cast<int32_t>(upperWord) << 16) | lowerWord;

            if(ExtraFlags > 0)
            {
                return FeaturePtr(p_.andMask(0xffff'ffff'ffff'fffc) +
                    (static_cast<int32_t>(member_ & 0xffff'fff8) >> 1));
            }
            else
            {
                return FeaturePtr(p_ + (member_ >> 1));
            }
        }
    }

    FeatureStore* store_;
    Tip currentTip_;
    Tex currentTex_;
    int32_t member_;
    DataPtr p_;
    DataPtr pExports_;
};


template<typename Derived, int ExtraFlags, int Step, bool Filtered=false>
class RelatedIterator : public RelatedIteratorBase<Derived, ExtraFlags, Step>
{

};

template<typename Derived, int ExtraFlags, int Step>
class RelatedIterator<Derived,ExtraFlags,Step,true> :
    public RelatedIteratorBase<Derived, ExtraFlags, Step>
{
protected:
    const MatcherHolder* matcher_;
    const Filter* filter_;
};


// \endcond
} // namespace geodesk

