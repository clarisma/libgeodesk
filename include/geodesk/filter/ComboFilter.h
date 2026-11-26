// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/filter/SpatialFilter.h>
#include <cstdint>
#include <span>

namespace geodesk {

class RoleFilter;

///
/// \cond lowlevel
///
class ComboFilter : public SpatialFilter
{
public:
    ~ComboFilter() override;
    ComboFilter(const ComboFilter&) = delete;
    ComboFilter& operator=(const ComboFilter&) = delete;

    /// Deallocates the filter including its trailing storage.
    static void operator delete(void* p) noexcept
    {
        ::operator delete(p);
    }

    /// Combines the two filters.
    ///
    /// @param a borrowed ref, non-null
    /// @param b borrowed ref, non-null
    /// @returns the new ComboFilter, or nullptr if the combination
    ///   of filters would not accept any features
    ///
   static const ComboFilter* create(const Filter* a, const Filter* b);

    /// Attempts to combine two filters (which may be null).
    /// Adds new references to a or b if they are non-null
    ///
    /// @param a borrowed ref, may be null
    /// @param b borrowed ref, may be null
    /// @returns true if res contains a filter or nullptr,
    ///   or false if the filter combination would not match any
    ///   features
    ///
    static bool tryCombine(const Filter** res, const Filter* a, const Filter* b)
    {
        if (!a)
        {
            if (b) b->addref();
            *res = b;
            return true;
        }
        if (!b)
        {
            a->addref();
            *res = a;
            return true;
        }
        const Filter* combined = create(a,b);
        *res = combined;
        return combined != nullptr;
    }

    bool accept(FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const override;
    int acceptTile(Tile tile) const override;
    const RoleFilter* roleFilter() const;
    const Filter* withoutRoleFilter() const;

    static const ComboFilter* cast(const Filter* filter)
    {
        assert(filter);
        assert (filter->flags() & FilterFlags::COMBO_FILTER);
        return reinterpret_cast<const ComboFilter*>(filter);
    }

private:
    std::span<const Filter* const> children() const noexcept
    {
        return { reinterpret_cast<const Filter* const*>(
            reinterpret_cast<uintptr_t>(this) +
                sizeof(ComboFilter)), totalCount_ };
    }

    std::span<const Filter*> children() noexcept
    {
        return { reinterpret_cast<const Filter**>(
            reinterpret_cast<uintptr_t>(this) +
                sizeof(ComboFilter)), totalCount_ };
    }

    ComboFilter(int flags, FeatureTypes acceptedTypes, const Box& bounds,
        uint32_t totalCount, uint32_t roleFilterCount) :
        SpatialFilter(flags, acceptedTypes, bounds),
        totalCount_(totalCount),
        roleFilterCount_(roleFilterCount) {}

    uint32_t totalCount_;
    uint32_t roleFilterCount_;
};

// \endcond
} // namespace geodesk
