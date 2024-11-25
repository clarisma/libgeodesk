// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <unordered_set>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/feature/Tip.h>
#include <geodesk/geom/Box.h>
#include <geodesk/geom/Tile.h>

namespace geodesk {

class Filter;

/// \cond lowlevel

/// @brief A class that traverses the Tile Index Tree of a
/// FeatureStore in an iterator-like fashion, returning all
/// tiles that intersect a given bounding box.
///
/// Tile traversal always starts at the root tile (TIP 1, 0/0/0),
/// which is neither rejected not accelerated, and by definition
/// does not have any NW neighbors.
///
/// Important: Index-walking changes in v2 -- since the root tile is
/// always included, we no longer call next() to obtain the root
/// tile; the TIW always starts in a valid state, and hence we
/// call next() *after* we've processed the root tile.
///
/// TODO: If a GOL only contains a single tile (the root tile),
///  make sure the TileIndexBuilder creates an empty child mask
///  (i.e. the TI must contain 3 slots: count, root tile, root
///  tile child mask)
///
class TileIndexWalker
{
public:
    TileIndexWalker(DataPtr pIndex, uint32_t zoomLevels,
        const Box& box, const Filter* filter);

    /// Moves to the next tile (depth-first).
    ///
    /// @return `true` if another accepted tile exists,
    ///   or `false` if traversal reached the end
    ///
    bool next();

    /// @brief If the current tile has children, ensures
    /// that any subsequent call to next() does not visit them.
    ///
    void skipChildren()
    {
        const Level& level = levels_[currentLevel_];
        currentLevel_ -= (level.currentCol < level.startCol);
    }

    Tip currentTip() const { return Tip(currentTip_); }
    Tile currentTile() const { return currentTile_; }
    uint32_t northwestFlags() const { return northwestFlags_; }
    uint32_t turboFlags() const { return turboFlags_; }
    const Box& bounds() const { return box_; }

private:
    static const int MAX_LEVELS = 13;   // currently 0 - 12
        // TODO: Limit to 8 levels in GOL 2.0?

    // TODO: uint16 supports max level 15 (not 16, because of sign;
    //  we start columns at -1)
	struct Level
	{
		uint64_t childTileMask;
        int32_t pChildEntries;
        Tile topLeftChildTile;
        uint16_t step;       // difference in zoom level to parent
        int16_t startCol;
        int16_t endCol;
        int16_t endRow;
        int16_t currentCol;
        int16_t currentRow;
        uint32_t turboFlags;
	};

    void startLevel(Level* level, int tip);
    void startRoot();
    
    Box box_;
    const Filter* filter_;
    DataPtr pIndex_;
    int currentLevel_;
    Tile currentTile_;
    uint32_t currentTip_;
    uint32_t northwestFlags_;
    uint32_t turboFlags_;
    bool tileBasedAcceleration_;
    bool trackAcceptedTiles_;
    std::unordered_set<Tile> acceptedTiles_;
    Level levels_[MAX_LEVELS];
        // TODO: can we drop one level? (Zoom 12 cannot have children)
};

// \endcond
} // namespace geodesk
