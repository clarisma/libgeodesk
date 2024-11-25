// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/query/TileIndexWalker.h>
#include <clarisma/util/Bits.h>
#include <geodesk/filter/Filter.h>

namespace geodesk {

using namespace clarisma;

// TODO: Check single-tile GOLs

TileIndexWalker::TileIndexWalker(
    DataPtr pIndex, uint32_t zoomLevels, const Box& box, const Filter* filter) :
    box_(box),
    filter_(filter),
    pIndex_(pIndex),
	currentLevel_(0),
    currentTile_(Tile::fromColumnRowZoom(0,0,0)),
    currentTip_(1),
    northwestFlags_(0),
    turboFlags_(0),
    tileBasedAcceleration_(false),
    trackAcceptedTiles_(false)
{
	int zoom = 0;
    zoomLevels >>= 1;
    Level* level = levels_;
    for (;;)
    {
        int step = Bits::countTrailingZerosInNonZero(zoomLevels) + 1;
        zoom += step;
        level->topLeftChildTile = Tile::fromColumnRowZoom(0, 0, zoom);
        level->step = step;
        zoomLevels >>= step;
        if (zoomLevels == 0) break;
        level++;
    }
    if (filter)
    {
        int filterFlags = filter->flags();
        if (filterFlags & FilterFlags::FAST_TILE_FILTER)
        {
            tileBasedAcceleration_ = true;
            if ((filterFlags & FilterFlags::STRICT_BBOX) == 0)
            {
                trackAcceptedTiles_ = true;
            }
        }
    }
    startLevel(&levels_[0], 1);
}

bool TileIndexWalker::next()
{
    Level* level = &levels_[currentLevel_];
    uint64_t childTileMask = level->childTileMask;
    for (;;)
    {
        // TODO: could restructure so we don't overflow uint16
        //  by checking *before* incrementing
        //  (only relevant if we ever support zoom levels up to 16,
        //  because then we'll need a uint16_t to represent all
        //  columns and rows

        level->currentCol++;
        if (level->currentCol > level->endCol)
        {
            level->currentRow++;
            if (level->currentRow > level->endRow)
            {
                if (currentLevel_ == 0)
                {
                    // We've completed the root; we are done
                    return false;
                }
                // continue with parent level
                currentLevel_--;
                level--;
                childTileMask = level->childTileMask;
                continue;
            }
            level->currentCol = level->startCol;
        }
        int childNumber = (level->currentRow << level->step) + level->currentCol;
        if ((childTileMask & (1LL << childNumber)) != 0)
        {
            // If the bit in the childTileMask is set,
            // this means that there is actually a tile
            // at this cell in the matrix
            // In the tile index, empty cells are skipped;
            // if we have a 4x4 matrix, and the mask bits
            // are 0b0000_0000_0011_0100, this means the
            // record is laid out like this:
            //
            // [parent tile]
            // [childTileMask]
            // [child at row0, col2]
            // [child at row1, col0]
            // [child at row1, col1]

            int childEntry = static_cast<int>(Bits::bitCount(childTileMask
                << (63 - childNumber))) - 1;
            // cannot shift by 64; only the lowest 5 bits count
            // TODO: could avoid -1 by setting pChildEntries one word earlier

            // by counting how many bits are set in the
            // mask before the bit at childNumber, we
            // determine the position of this child's
            // entry (This should be a very fast operation
            // on modern CPUs)

            currentTile_ = level->topLeftChildTile.relative(
                level->currentCol, level->currentRow);
            // Log.debug("TIW: Current tile %s, Filter = %s", Tile.toString(currentTile), filter);

            if (tileBasedAcceleration_)
            {
                // TODO: Don't call acceptTile() if all turbo-flags are
                //  set for the current tile
                //  If the parent is fast-accept, children by definition
                //  are fast-accept as well
                //  It's more complicated if there are multiple
                //  spatial filters, as a tile may be accelerated for
                //  some but not others
                
                int turboFlags = filter_->acceptTile(currentTile_);
                if (turboFlags < 0) continue;
                turboFlags_ = static_cast<uint32_t>(turboFlags);
                
                if (trackAcceptedTiles_)
                {
                    Tile northTile = currentTile_.neighbor(0, -1);
                    Tile westTile = currentTile_.neighbor(-1, 0);
                    northwestFlags_ =
                        (acceptedTiles_.find(northTile) != acceptedTiles_.end() ?
                            FeatureFlags::MULTITILE_NORTH : 0) |
                        (acceptedTiles_.find(westTile) != acceptedTiles_.end() ?
                            FeatureFlags::MULTITILE_WEST : 0);
                    acceptedTiles_.insert(currentTile_);
                }
                else
                {
                    // If we're not tracking accepted NW tiles (for filters that
                    // use a strict bbox), pretend that NW tiles exist
                    // If a feature extends into a N/W tile, the query bbox must
                    // extend into the N/W tile as well, else it cannot be fully
                    // within the bbox
                    // (For simplicity, we could track tiles for strict-bbox filters
                    // as well)

                    northwestFlags_ = FeatureFlags::MULTITILE_NORTH | FeatureFlags::MULTITILE_WEST;
                }
            }
            else
            {
                // If we're processing a dense set of tiles, calculate the
                // northwestFlags based on query bbox
                // TODO: There's probably a cheaper way to calculate this

                northwestFlags_ =
                    ((box_.maxY() > currentTile_.topY()) ?
                        FeatureFlags::MULTITILE_NORTH : 0) |
                    ((box_.minX() < currentTile_.leftX()) ?
                        FeatureFlags::MULTITILE_WEST : 0);
                turboFlags_ = 0;
            }
            int tip = level->pChildEntries + childEntry;
            uint32_t pageOrPtr = (pIndex_ + (tip << 2)).getUnsignedInt();
            if ((pageOrPtr & 3) == 1)
            {
                // Changed for v2: The lowest 2 bits
                //  are flags. A value of 01 indicates a pointer
                //  to a child level
                //  TODO: Use TileIndexEntry class

                // current tile has children: prepare to move up to the
                // next level in the tile tree

                currentLevel_++;
                tip += (static_cast<int32_t>(pageOrPtr) ^ 1) >> 2;
                startLevel(level+1, tip);
            }
            currentTip_ = tip;
            return true;
        }
    }
}

void TileIndexWalker::startLevel(Level* level, int tip)
{
    int zoom = level->topLeftChildTile.zoom();
    int step = level->step;
    int extent = 1 << step;     
    int tileTop = currentTile_.row() << step;
    int tileLeft = currentTile_.column() << step;
    level->topLeftChildTile = Tile::fromColumnRowZoom(tileLeft, tileTop, zoom);
    int left = Tile::columnFromXZ(box_.minX(), zoom);
    int right = Tile::columnFromXZ(box_.maxX(), zoom);
    int top = Tile::rowFromYZ(box_.maxY(), zoom);
    int bottom = Tile::rowFromYZ(box_.minY(), zoom);
    level->startCol = std::max(left - tileLeft, 0);
    int startRow = std::max(top - tileTop, 0);
    level->endCol = std::min(right - tileLeft, extent - 1);
    level->endRow = std::min(bottom - tileTop, extent - 1);
    level->currentCol = level->startCol - 1;
    level->currentRow = startRow;

    level->childTileMask = (pIndex_ + (tip + 1) * 4).getUnsignedLong();
        // TODO: Is this unaligned???
    level->pChildEntries = tip + (step == 3 ? 3 : 2);
    level->turboFlags = 0; // TODO
}

} // namespace geodesk
