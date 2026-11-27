// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/filter/ComboFilter.h>

namespace geodesk {

// Filter combination rules:
// - Combined filter is FAST_TILE_FILTER if *either* is FAST_TILE_FILTER
// - Combined filter is USES_BBOX if *either* USES_BBOX
// - Combined filter is STRICT_BBOX if *both* are STRICT_BBOX
// - If combined filter is USES_BBOX:
//   - If STRICT_BBOX, use intersection of all bboxes
//   - If not STRICT_BBOX, use smallest bbox
// -  

const ComboFilter* ComboFilter::create(const Filter* a, const Filter* b)
{
    assert(a);
    assert(b);

    FeatureTypes types = a->acceptedTypes() & b->acceptedTypes();
    if (types==0) [[unlikely]]
    {
        return nullptr;
    }

    int aFlags = a->flags();
    int bFlags = b->flags();
    int aComboFlag = aFlags & FilterFlags::COMBO_FILTER;
    int bComboFlag = bFlags & FilterFlags::COMBO_FILTER;
    uint32_t totalCount =
        (aComboFlag ? cast(a)->totalCount_ : 1) +
        (bComboFlag ? cast(b)->totalCount_ : 1);
    uint32_t roleFilterCount =
        (aComboFlag ? cast(a)->roleFilterCount_ :
            ((aFlags & FilterFlags::ROLE_FILTER) ? 1 : 0)) +
        (bComboFlag ? cast(b)->roleFilterCount_ :
            ((bFlags & FilterFlags::ROLE_FILTER) ? 1 : 0));

    int flags = (aFlags | bFlags) & (FilterFlags::FAST_TILE_FILTER) |
        FilterFlags::COMBO_FILTER | FilterFlags::USES_BBOX;

    Box bounds;
    if ((aFlags & bFlags) & FilterFlags::USES_BBOX)
    {
        // both filters are spatial
        if ((aFlags | bFlags) & FilterFlags::STRICT_BBOX)
        {
            // If *either* is strict-bbox, the spatial-filter bbox
            // if the intersection of both

            bounds = Box::simpleIntersection(
                a->getBounds(), b->getBounds());
            if (bounds.isEmpty())
            {
                return nullptr;
            }

            // The result is only strict-bbox if both are strict-bbox
            flags |= aFlags & bFlags & FilterFlags::STRICT_BBOX;
        }
        else
        {
            bounds = Box::simpleSmaller(a->getBounds(), b->getBounds());
        }
    }
    else
    {
        // one one (or neither) is spatial
        if (aFlags & FilterFlags::USES_BBOX)
        {
            bounds = SpatialFilter::cast(a)->bounds();
            flags |= aFlags & FilterFlags::STRICT_BBOX;
        }
        else if (bFlags & FilterFlags::USES_BBOX)
        {
            bounds = SpatialFilter::cast(b)->bounds();
            flags |= bFlags & FilterFlags::STRICT_BBOX;
        }
        else
        {
            bounds = Box::ofWorld();
        }
    }

    size_t bytes = sizeof(ComboFilter) + totalCount * sizeof(const ComboFilter*);
    void* memory = ::operator new(bytes);
    auto storage = ::new (memory) std::byte[bytes];
    auto dest = ::new (storage + sizeof(ComboFilter))
        const Filter*[totalCount];
    ComboFilter* combo = new(memory)ComboFilter(
        flags, types, bounds, totalCount, roleFilterCount);

    using Children = std::span<const Filter* const>;
    Children aChildren = aComboFlag ?
        cast(a)->children() : Children(&a, 1);
    Children bChildren = bComboFlag ?
        cast(b)->children() : Children(&b, 1);

    for (const Filter* child : aChildren)
    {
        child->addref();
        *dest++ = child;
    }
    for (const Filter* child : bChildren)
    {
        child->addref();
        *dest++ = child;
    }
    return combo;
}


ComboFilter::~ComboFilter()
{
    for (const Filter* child : children())
    {
        child->release();
    }
}


bool ComboFilter::accept(const FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const
{
    for (const Filter* child : children())
    {
        FastFilterHint fastChild = fast;
        fastChild.turboFlags &= 1;      // mask of the turbo flag for this child
        if (!child->accept(store, feature, fastChild)) return false;
        fast.turboFlags >>= 1;          // bring the next turbo flag to bit position 0
    }
    return true;
}

// TODO: check only the first 31 children
int ComboFilter::acceptTile(Tile tile) const
{
    int fast = 0;
    int nextFlag = 1;
    for (const Filter* child : children())
    {
        int accept = child->acceptTile(tile);
        if (accept < 0) return accept;
        fast |= accept == 0 ? 0 : nextFlag;
        nextFlag <<= 1;
    }
    return fast;
}


const RoleFilter* ComboFilter::roleFilter() const
{
    for (const Filter* child : children())
    {
        if (child->isRoleFilter()) return reinterpret_cast<const RoleFilter*>(child);
    }
    return nullptr;
}

const Filter* ComboFilter::withoutRoleFilter() const
{
    uint32_t nonRoleFilterCount = totalCount_ - roleFilterCount_;
    if (nonRoleFilterCount == 0) [[unlikely]] return nullptr;
    if (nonRoleFilterCount == 1) [[likely]]
    {
        for (const Filter* child : children())
        {
            if (!child->isRoleFilter())
            {
                child->addref();
                return child;
            }
        }
        assert(false);
    }

    size_t bytes = sizeof(ComboFilter) + nonRoleFilterCount *
        sizeof(const ComboFilter*);
    void* memory = ::operator new(bytes);
    auto storage = ::new (memory) std::byte[bytes];
    auto dest = ::new (storage + sizeof(ComboFilter))
        const Filter*[nonRoleFilterCount];
    auto p = dest;
    ComboFilter* newCombo = new(memory)ComboFilter(
        flags(), acceptedTypes(), bounds(), nonRoleFilterCount, 0);
    for (const Filter* child : children())
    {
        if (!child->isRoleFilter())
        {
            child->addref();
            *p++ = child;
        }
    }
    assert(p - dest == nonRoleFilterCount);
    return newCombo;
}

} // namespace geodesk
