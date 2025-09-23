// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/FeaturePtr.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/feature/Tex.h>
#include <geodesk/feature/TileConstants.h>
#include <geodesk/filter/Filter.h>

namespace geodesk {

/// \cond lowlevel
///

// TODO: Use std::conditional_t
//  e.g.
//   using MatcherType = std::conditional_t<Filtered, const MatcherHolder*, std::nullptr_t>;
//   using FilterType = std::conditional_t<Filtered, const Filter*, std::nullptr_t>;

template<typename Derived, typename Ptr, int ExtraFlags, int Step>
class RelatedIteratorBase
{
public:
    RelatedIteratorBase(FeatureStore* store, DataPtr p, Tex initialTex) :
        store_(store),
        currentTip_(FeatureConstants::START_TIP),
        currentTex_(initialTex),
        member_(0),
        missingTiles_(false),
        p_(p)
    {
    }

    FeatureStore* store() const { return store_; }

    Ptr next()
    {
        while ((member_ & MemberFlags::LAST) == 0)
        {
            DataPtr pCurrent = p_;
            uint16_t lowerWord = p_.getUnsignedShort();
            p_ += Step;
            if (lowerWord & MemberFlags::FOREIGN)
            {
                if(lowerWord & (1 << (3 + ExtraFlags)))  [[unlikely]]
                {
                    // wide TEX delta
                    uint16_t upperWord = p_.getUnsignedShort();
                    p_ += Step;
                    member_ = (static_cast<int32_t>(upperWord) << 16) | lowerWord;
                }
                else
                {
                    member_ = static_cast<int16_t>(lowerWord);
                }
                if(isInDifferentTile())
                {
                    // foreign member in different tile
                    int32_t tipDelta = p_.getShort();
                    p_ += Step;
                    if (tipDelta & 1)
                    {
                        // wide TIP delta
                        tipDelta = (tipDelta & 0xffff) |
                            (static_cast<int32_t>(p_.getShort()) << 16);
                        p_ += Step;
                    }
                    tipDelta >>= 1;     // signed
                    currentTip_ += tipDelta;
                    pExports_ = nullptr;
                }
                currentTex_ += member_ >> (4 + ExtraFlags);
            }
            else
            {
                uint16_t upperWord = p_.getUnsignedShort();
                p_ += Step;
                member_ = (static_cast<int32_t>(upperWord) << 16) | lowerWord;
            }
            if(!self()->readAndAcceptRole()) continue;
            FeaturePtr feature;
            if (lowerWord & MemberFlags::FOREIGN)
            {
                if(!pExports_)
                {
                    TilePtr pTile = store_->fetchTile(currentTip_);
                    if(!pTile)  [[unlikely]]
                    {
                        // TODO: set flag to indicate missing tiles
                        continue;
                    }
                    pExports_ = (pTile + TileConstants::EXPORTS_OFS).follow();
                }
                feature = FeaturePtr((pExports_ + static_cast<int>(currentTex_) * 4).follow());
            }
            else
            {
                if(ExtraFlags > 0)
                {
                    feature = FeaturePtr(pCurrent.andMask(0xffff'ffff'ffff'fffc) +
                        (static_cast<int32_t>(member_ & 0xffff'fff8) >> 1));
                }
                else
                {
                    feature = FeaturePtr(pCurrent + (member_ >> 1));
                }
            }
            if(self()->accept(feature)) return Ptr(feature);
        }
        return Ptr();
    }

    bool isForeign() const { return member_ & MemberFlags::FOREIGN; }
    bool isInDifferentTile() const
    {
        assert(isForeign());
        return member_ & (1 << (2 + ExtraFlags));
    }

    Tip tip() const { return currentTip_; }
    Tex tex() const { return currentTex_; }

protected:
    Derived* self() noexcept { return static_cast<Derived*>(this); }

    bool readAndAcceptRole()    // CRTP override
    {
        return true;
    }

    bool accept(FeaturePtr feature) const   // CRTP override
    {
        return true;
    }

    FeatureStore* store_;
    Tip currentTip_;
    Tex currentTex_;
    int32_t member_;
    bool missingTiles_;
    DataPtr p_;
    DataPtr pExports_;
};



template<typename Derived, typename Ptr, int ExtraFlags, int Step>
class RelatedIterator : public RelatedIteratorBase<Derived,Ptr,ExtraFlags,Step>
{
public:
    RelatedIterator(FeatureStore* store, DataPtr p, Tex initialTex,
        const MatcherHolder* matcher, const Filter* filter) :
        RelatedIteratorBase<Derived,Ptr,ExtraFlags,Step>(store, p, initialTex),
        matcher_(matcher),
        filter_(filter)
    {
    }

    bool accept(FeaturePtr feature) const   // CRTP override
    {
        if (!matcher_->mainMatcher().accept(feature)) return false;
        return filter_ == nullptr || filter_->accept(
            this->store_, feature, FastFilterHint());
    }

protected:
    const MatcherHolder* matcher_;
    const Filter* filter_;
};


// \endcond
} // namespace geodesk

