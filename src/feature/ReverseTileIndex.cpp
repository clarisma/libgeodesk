// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/ReverseTileIndex.h>
#include <cassert>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/query/TileIndexWalker.h>

namespace geodesk {

ReverseTileIndex::~ReverseTileIndex()
{
    delete[] index_.load(std::memory_order_relaxed);
}

const uint32_t* ReverseTileIndex::initialize() const
{
    std::lock_guard lock(indexMutex_);
    const uint32_t* index = index_.load(std::memory_order_relaxed);
    if (index != nullptr) return index;

    int tipCount = store_->tipCount();
    uint32_t* newIndex = new uint32_t[tipCount + 1];
        // tipCount does not include entry 0
    newIndex[0] = static_cast<uint32_t>(tipCount);
    Box world = Box::ofWorld();
    TileIndexWalker tiw(store_->tileIndex(), store_->zoomLevels(),
        world, nullptr);
    do
    {
        assert(tiw.currentTip() <= tipCount);
            // tipCount does not include entry 0
        newIndex[tiw.currentTip()] = static_cast<uint32_t>(tiw.currentTile());
    }
    while (tiw.next());
    index_.store(newIndex, std::memory_order_release);
    return newIndex;
}


} // namespace geodesk