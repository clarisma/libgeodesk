// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/IdIndex.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/feature/TileConstants.h>
#include <geodesk/query/TileIndexWalker.h>
#include <geodesk/geom/Box.h>
#include <filesystem>

namespace fs = std::filesystem;
using namespace TileConstants;

namespace geodesk {

int IdIndex::calculateBaseBitWidth(uint32_t tileCount)
{
    // Same formula as gol-tool: 32 - countLeadingZeros(tileCount)
    // This gives the minimum bits needed to represent values 0..tileCount
    if (tileCount == 0) return 1;
    int leadingZeros = 0;
    uint32_t n = tileCount;
    if (n <= 0x0000FFFF) { leadingZeros += 16; n <<= 16; }
    if (n <= 0x00FFFFFF) { leadingZeros += 8;  n <<= 8; }
    if (n <= 0x0FFFFFFF) { leadingZeros += 4;  n <<= 4; }
    if (n <= 0x3FFFFFFF) { leadingZeros += 2;  n <<= 2; }
    if (n <= 0x7FFFFFFF) { leadingZeros += 1; }
    return 32 - leadingZeros;
}

IdIndex::IdIndex(FeatureStore* store) : store_(store)
{
    // Get GOL path from parent class FreeStore::fileName()
    std::string golPath = store->fileName();

    // Strip .gol extension if present
    if (golPath.size() > 4 && golPath.substr(golPath.size() - 4) == ".gol")
    {
        golPath = golPath.substr(0, golPath.size() - 4);
    }

    std::string indexDir = golPath + "-indexes";

    fs::path nodePath = fs::path(indexDir) / "nodes.idx";
    fs::path wayPath = fs::path(indexDir) / "ways.idx";
    fs::path relPath = fs::path(indexDir) / "relations.idx";

    // Check if all three index files exist
    if (!fs::exists(nodePath) || !fs::exists(wayPath) || !fs::exists(relPath))
    {
        return;  // available_ stays false
    }

    try
    {
        // Calculate bit width based on tile count (same formula as gol-tool)
        int baseBits = calculateBaseBitWidth(store->tileCount());
        int nodeBits = baseBits;
        int wayBits = baseBits + PILEPAIR_EXTRA_BITS;
        int relBits = baseBits + PILEPAIR_EXTRA_BITS;

        // Open each index with its correct bit width:
        // - nodes.idx: baseBits (pile only)
        // - ways.idx: baseBits + 2 (pile << 2 | tile_pair_flags)
        // - relations.idx: baseBits + 2 (pile << 2 | tile_pair_flags)
        nodeIndex_.open(nodePath.string().c_str(),
            clarisma::FileHandle::OpenMode::READ, nodeBits);
        wayIndex_.open(wayPath.string().c_str(),
            clarisma::FileHandle::OpenMode::READ, wayBits);
        relationIndex_.open(relPath.string().c_str(),
            clarisma::FileHandle::OpenMode::READ, relBits);

        buildPileToTip();
        available_ = true;
    }
    catch (...)
    {
        // If any file fails to open, leave available_ = false
    }
}

void IdIndex::buildPileToTip()
{
    maxPile_ = store_->tileCount();
    pileToTip_ = std::make_unique<Tip[]>(maxPile_ + 1);
    pileToTip_[0] = Tip();  // Pile 0 is invalid/not-found sentinel

    // gol-tool assigns pile numbers using TileIndexWalker traversal order
    // (depth-first). We must use the same order to correctly map pile -> tip.
    // See gol-tool's TileCatalog constructor for the canonical implementation.

    TileIndexWalker tiw(store_->tileIndex(), store_->zoomLevels(),
        Box::ofWorld(), nullptr);
    uint32_t pile = 0;
    do
    {
        pile++;
        pileToTip_[pile] = tiw.currentTip();
    }
    while (tiw.next() && pile < maxPile_);
}

FeaturePtr IdIndex::findById(uint64_t id, FeatureType type)
{
    if (!available_) return FeaturePtr();

    // Select the appropriate index file based on feature type
    clarisma::IndexFile* index;
    switch (type)
    {
    case FeatureType::NODE:
        index = &nodeIndex_;
        break;
    case FeatureType::WAY:
        index = &wayIndex_;
        break;
    case FeatureType::RELATION:
        index = &relationIndex_;
        break;
    default:
        return FeaturePtr();
    }

    // Look up value from the index
    uint32_t indexValue = index->get(id);
    if (indexValue == 0) return FeaturePtr();

    // For ways and relations, the index stores a "pilePair" which is
    // (pile << 2) | tile_pair_flags. We need to right-shift by 2 to get the pile.
    // For nodes, the index stores the pile directly.
    uint32_t pile = (type == FeatureType::NODE) ? indexValue : (indexValue >> 2);

    if (pile == 0 || pile > maxPile_) return FeaturePtr();

    // Convert pile to tip using our depth-first mapping
    Tip tip = pileToTip_[pile];
    TilePtr tile = store_->fetchTile(tip);
    if (!tile) return FeaturePtr();

    return scanTileForId(tile, id, type);
}

FeaturePtr IdIndex::scanTileForId(TilePtr tile, uint64_t id, FeatureType type) const
{
    FeaturePtr result;

    if (type == FeatureType::NODE)
    {
        result = scanIndexForId(tile + NODE_INDEX_OFS, id, type);
    }
    else if (type == FeatureType::WAY)
    {
        // Ways can be in WAY_INDEX or AREA_INDEX (if closed/area)
        result = scanIndexForId(tile + WAY_INDEX_OFS, id, type);
        if (result.isNull())
        {
            result = scanIndexForId(tile + AREA_INDEX_OFS, id, type);
        }
    }
    else if (type == FeatureType::RELATION)
    {
        // Relations can be in RELATION_INDEX or AREA_INDEX (if multipolygon)
        result = scanIndexForId(tile + RELATION_INDEX_OFS, id, type);
        if (result.isNull())
        {
            result = scanIndexForId(tile + AREA_INDEX_OFS, id, type);
        }
    }

    return result;
}

FeaturePtr IdIndex::scanIndexForId(DataPtr pIndex, uint64_t id, FeatureType type) const
{
    // Get root pointer from index offset
    int32_t ptr = pIndex.getInt();
    if (ptr == 0) return FeaturePtr();  // No features of this type in tile

    // Walk all key-index branches (no key filtering for ID lookup)
    DataPtr p = pIndex + ptr;
    bool isNode = (type == FeatureType::NODE);

    for (;;)
    {
        ptr = p.getInt();
        int32_t last = ptr & 1;

        // Scan this branch (ptr ^ last clears the last bit)
        FeaturePtr found = scanBranchForId(p + (ptr ^ last), id, type, isNode);
        if (!found.isNull()) return found;

        if (last != 0) break;
        p += 8;  // Key-index entries are 8 bytes
    }

    return FeaturePtr();
}

FeaturePtr IdIndex::scanBranchForId(DataPtr p, uint64_t id, FeatureType type, bool isNode) const
{
    // Entry size: 20 bytes for all types (ptr + 16-byte bbox)
    constexpr int BRANCH_ENTRY_SIZE = 20;

    for (;;)
    {
        int32_t ptr = p.getInt();
        int32_t last = ptr & 1;

        // Get child pointer (clear low 2 bits: last flag and leaf flag)
        DataPtr pChild = p + (ptr & 0xffff'fffc);

        FeaturePtr found;
        if (ptr & 2)
        {
            // Leaf node
            if (isNode)
            {
                found = scanNodeLeafForId(pChild, id);
            }
            else
            {
                found = scanLeafForId(pChild, id, type);
            }
        }
        else
        {
            // Branch node - recurse
            found = scanBranchForId(pChild, id, type, isNode);
        }

        if (!found.isNull()) return found;
        if (last != 0) break;
        p += BRANCH_ENTRY_SIZE;
    }

    return FeaturePtr();
}

FeaturePtr IdIndex::scanNodeLeafForId(DataPtr p, uint64_t id) const
{
    // Node leaf entry layout:
    // p+0: x coordinate (4 bytes)
    // p+4: y coordinate (4 bytes)
    // p+8: flags (4 bytes) - FeaturePtr starts here
    // Entry size: 20 + (flags & 4) bytes

    for (;;)
    {
        int32_t flags = (p + 8).getInt();

        // FeaturePtr for nodes is at p+8 (where flags are)
        FeaturePtr feature(p + 8);

        if (feature.id() == id)
        {
            return feature;
        }

        if (flags & 1) break;  // Last entry flag

        // Entry size: 20 bytes + 4 extra if relation member (flags bit 2)
        p += 20 + (flags & 4);
    }

    return FeaturePtr();
}

FeaturePtr IdIndex::scanLeafForId(DataPtr p, uint64_t id, FeatureType type) const
{
    // Way/Relation leaf entry layout:
    // p+0:  minX (4 bytes)
    // p+4:  minY (4 bytes)
    // p+8:  maxX (4 bytes)
    // p+12: maxY (4 bytes)
    // p+16: flags (4 bytes) - FeaturePtr starts here
    // Entry size: 32 bytes fixed

    for (;;)
    {
        int32_t flags = (p + 16).getInt();

        // FeaturePtr for ways/relations is at p+16
        FeaturePtr feature(p + 16);

        if (feature.id() == id)
        {
            // For AREA index, verify the type matches (contains both ways and relations)
            if (feature.type() == type)
            {
                return feature;
            }
        }

        if (flags & 1) break;  // Last entry flag
        p += 32;
    }

    return FeaturePtr();
}

} // namespace geodesk
