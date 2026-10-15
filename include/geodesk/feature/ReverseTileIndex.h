// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <atomic>
#include <cassert>
#include <cstdint>
#include <mutex>
#include <geodesk/geom/Tile.h>
#include <geodesk/feature/Tip.h>

namespace geodesk {

class FeatureStore;

/// Provides lazy reverse lookup from TIPs to tiles.
///
/// The associated FeatureStore must outlive this object. The
/// object itself must not be destroyed while another thread
/// is performing a lookup.
///
class ReverseTileIndex 
{
public:
    explicit ReverseTileIndex(const FeatureStore* store) noexcept : store_(store) {}
    ~ReverseTileIndex();

    class Ptr
    {
    public:
        explicit Ptr(const uint32_t* index) : index_(index) {}

        uint32_t tipCount() const noexcept
        {
            return index_[0];
        }

        bool isNull() const noexcept { return index_ == nullptr; }

        /// Returns the `Tile` that corresponds to the given TIP.
        /// The given TIP must be valid for the associated store,
        /// or the lookup will be undefined.
        ///
        /// @param tip a valid TIP in the FeatureStore's tile index
        /// @return the TIP's `Tile`
        ///
        Tile lookupFast(Tip tip) const
        {
            assert(!isNull());
            assert(!tip.isNull());
            assert(tip <= tipCount());
            return Tile(index_[tip]);
        }

    private:
        const uint32_t* index_;
    };

    Ptr index() const noexcept
    {
        const uint32_t* index = index_.load(std::memory_order_acquire);
        if (index == nullptr) [[unlikely]]
        {
            index = initialize();
        }
        return Ptr(index);
    }

    /// Returns the `Tile` that corresponds to the given TIP.
    /// The given TIP must be valid for the associated store,
    /// or the lookup will be undefined.
    ///
    /// @param tip a valid TIP in the FeatureStore's tile index
    /// @return the TIP's `Tile`
    ///
    Tile lookupFast(Tip tip) const
    {
        return index().lookupFast(tip);
    }

private:
    const uint32_t* initialize() const;

    mutable std::atomic<const uint32_t*> index_{nullptr};
    const FeatureStore* store_;
    mutable std::mutex indexMutex_;
};

} // namespace geodesk
