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

const Tile* ReverseTileIndex::initialize() const
{
    std::lock_guard lock(indexMutex_);
    const Tile* index = index_.load(std::memory_order_relaxed);
    if (index != nullptr) return index;
    Tile* newIndex = new Tile[store_->tipCount() + 1];
        // tipCount does not include entry 0
    Box world = Box::ofWorld();
    TileIndexWalker tiw(store_->tileIndex(), store_->zoomLevels(),
        world, nullptr);
    do
    {
        assert(tiw.currentTip() <= store_->tipCount());
            // tipCount does not include entry 0
        newIndex[tiw.currentTip()] = tiw.currentTile();
    }
    while (tiw.next());
    index_.store(newIndex, std::memory_order_release);
    return newIndex;
}


} // namespace geodesk