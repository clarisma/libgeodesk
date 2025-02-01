// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstdint>
#include <clarisma/text/Format.h>
#include "types.h"

namespace geodesk {

// internal; needed by GOL

/// A TEX Delta.
///
/// \cond lowlevel
///
class TexDelta
{
public:
    constexpr TexDelta() : delta_(0) {}
    constexpr TexDelta(int32_t delta) : delta_(delta) {}

    operator int32_t() const { return delta_; }

    bool isWide(int narrowBitsAvailable) const
    {
        int shift = 32 - narrowBitsAvailable;
        return ((delta_ << shift) >> shift) != delta_;
    }

    // TODO: Fix sign extension!!!
    int wideFlagInNodeTable()
    {
        // 12 bits signed
        return ((delta_ << 20 >> 20) != delta_) ? MemberFlags::WIDE_NODE_TEX : 0;
    }

    int wideFlagInRelationTable()
    {
        // 12 bits signed
        return ((delta_ << 20 >> 20) != delta_) ? MemberFlags::WIDE_RELATION_TEX : 0;
    }

    int wideFlagInMemberTable()
    {
        // 11 bits signed
        return ((delta_ << 21 >> 21) != delta_) ? MemberFlags::WIDE_MEMBER_TEX : 0;
    }


private:
    int32_t delta_;
};

/// A Tile Export Number.
///
/// @ingroup lowlevel
///
class Tex
{
public:
    constexpr Tex() : tex_(0) {}
    constexpr Tex(int32_t tex) : tex_(tex) {}
    constexpr Tex(uint32_t tex) : tex_(static_cast<int32_t>(tex)) {}

    // TODO: Should we make conversions explicit?

    constexpr operator int32_t () const
    {
        return tex_;
    }

    Tex& operator+=(TexDelta delta)
    {
        tex_ += delta;
        return *this;
    }

    TexDelta operator-(Tex other) const noexcept
    {
        return TexDelta(tex_ - other.tex_);
    }

    static constexpr int32_t MEMBERS_START_TEX = 0x400;
    static constexpr int32_t RELATIONS_START_TEX = 0x800;
    static constexpr int32_t WAYNODES_START_TEX = 0x800;

    static constexpr int32_t MEMBERS_TEX_BITS = 11;
    static constexpr int32_t RELATIONS_TEX_BITS = 12;
    static constexpr int32_t WAYNODES_TEX_BITS = 12;

private:
    int32_t tex_;
};

// \endcond
} // namespace geodesk
