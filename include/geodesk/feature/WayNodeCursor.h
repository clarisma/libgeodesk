// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/WayPtr.h>

namespace geodesk {

/// Cursor to iterate coordinates of a Way (and optionally their IDs).
/// Always starts with the first node already fetched; call next()
/// to iterate all additional nodes.
///
/// \cond lowlevel
///
class WayNodeCursor
{
public:
    /// Initializes the cursor for the given way; if the way is an area,
    /// the first node will be returned as the final node.
    ///
    /// @param way the way
    /// @param wayNodeIds whether to include waynode IDs (if false,
    ///   all nodes will have ID set to 0)
    ///
    WayNodeCursor(WayPtr way, bool wayNodeIds) :
        WayNodeCursor({way.minX(), way.minY()}, way.bodyptr(),
            way.isArea(), wayNodeIds)
    {
    }

    /// Initializes the cursor and fetches the first node.
    ///
    /// @param xy the base coordinate (the way's minX/minY)
    /// @param p  pointer to the way's body
    /// @param duplicateFirst whether to return the first node again
    ///    after we've read all of the stored nodes (used to obtain
    ///    the full geometry of area ways)
    /// @param wayNodeIds whether to include waynode IDs (if false,
    ///   all nodes will have ID set to 0)
    ///
    WayNodeCursor(Coordinate xy, const uint8_t* p, bool duplicateFirst, bool wayNodeIds)
    {
        remaining_ = clarisma::readVarint32(p) - 1;
        duplicateFirst_ = duplicateFirst;
        xy.x += clarisma::readSignedVarint32(p);
        xy.y += clarisma::readSignedVarint32(p);
        pCoords_ = p;
        currentXY_ = xy;
        firstXY_ = duplicateFirst ? xy : Coordinate();
        if (wayNodeIds)
        {
            clarisma::skipVarints(p, remaining_ * 2);
            currentId_ = clarisma::readSignedVarint64(p);
            firstId_ = duplicateFirst ? currentId_ : 0;
            pIds_ = p;
        }
        else
        {
            pIds_ = nullptr;
            currentId_ = 0;
            firstId_ = 0;
        }
    }

    Coordinate xy() const noexcept { return currentXY_; }
    uint64_t id() const noexcept { return currentId_; }

    /// Moves to the next node (if any remain)
    /// @returns true if another node has been fetched, or false
    ///   if we've read past the end
    ///
    bool next() noexcept
    {
        if (remaining_)
        {
            currentXY_.x += clarisma::readSignedVarint32(pCoords_);
            currentXY_.y += clarisma::readSignedVarint32(pCoords_);
            if (currentId_)
            {
                currentId_ += clarisma::readSignedVarint64(pIds_);
            }
            --remaining_;
            return true;
        }
        currentXY_ = firstXY_;
        currentId_ = firstId_;
        firstXY_ = Coordinate();
        firstId_ = 0;
        bool prevDuplicateFirst = duplicateFirst_;
        duplicateFirst_ = false;
        return prevDuplicateFirst;
    }

private:
    const uint8_t* pCoords_;
    int remaining_;
    bool duplicateFirst_;
    Coordinate currentXY_;
    Coordinate firstXY_;
    const uint8_t* pIds_;
    uint64_t currentId_;
    uint64_t firstId_;
};

// \endcond
} // namespace geodesk
