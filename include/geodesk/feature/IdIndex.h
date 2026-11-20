// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <memory>
#include <clarisma/store/IndexFile.h>
#include <geodesk/feature/FeaturePtr.h>
#include <geodesk/feature/Tip.h>
#include <geodesk/feature/TilePtr.h>
#include <geodesk/feature/types.h>

namespace geodesk {

class FeatureStore;

/// @brief Provides O(1) lookup of features by their OSM ID.
///
/// This class manages the optional ID index files created by `gol build -i`.
/// The index files map OSM IDs to pile numbers, which are then converted
/// to tile index positions (tips) for feature retrieval.
///
/// Lookup chain: OSM_ID -> pile -> tip -> tile -> linear scan
///
class IdIndex
{
public:
    /// Constructs an IdIndex for the given FeatureStore.
    /// If index files don't exist, isAvailable() returns false.
    explicit IdIndex(FeatureStore* store);

    ~IdIndex() = default;

    // Non-copyable, non-movable (owns file handles)
    IdIndex(const IdIndex&) = delete;
    IdIndex& operator=(const IdIndex&) = delete;
    IdIndex(IdIndex&&) = delete;
    IdIndex& operator=(IdIndex&&) = delete;

    /// Returns true if ID index files are available and loaded.
    bool isAvailable() const noexcept { return available_; }

    /// Finds a feature by its OSM ID.
    /// @param id The OSM ID to look up
    /// @param type The feature type (NODE, WAY, or RELATION)
    /// @return The FeaturePtr if found, or an empty FeaturePtr if not found
    FeaturePtr findById(uint64_t id, FeatureType type);

private:
    /// Extra bits for ways and relations to encode tile pair flags.
    static constexpr int PILEPAIR_EXTRA_BITS = 2;

    /// Calculates bit width needed to store tile count values.
    /// Uses same formula as gol-tool: 32 - countLeadingZeros(tileCount)
    static int calculateBaseBitWidth(uint32_t tileCount);

    /// Builds the pile-to-tip mapping by walking the tile index.
    void buildPileToTip();

    /// Scans a tile to find a feature by ID.
    FeaturePtr scanTileForId(TilePtr tile, uint64_t id, FeatureType type) const;

    /// Scans a single index (NODE, WAY, AREA, or RELATION) for a feature.
    FeaturePtr scanIndexForId(DataPtr pIndex, uint64_t id, FeatureType type) const;

    /// Recursively scans R-tree branches for a feature.
    FeaturePtr scanBranchForId(DataPtr p, uint64_t id, FeatureType type, bool isNode) const;

    /// Scans a leaf node for the feature.
    FeaturePtr scanNodeLeafForId(DataPtr p, uint64_t id) const;

    /// Scans a leaf (way/relation) for the feature.
    FeaturePtr scanLeafForId(DataPtr p, uint64_t id, FeatureType type) const;

    FeatureStore* store_;
    bool available_ = false;
    uint32_t maxPile_ = 0;
    std::unique_ptr<Tip[]> pileToTip_;
    clarisma::IndexFile nodeIndex_;
    clarisma::IndexFile wayIndex_;
    clarisma::IndexFile relationIndex_;
};

} // namespace geodesk
