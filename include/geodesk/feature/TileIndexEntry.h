// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

namespace geodesk {

// \cond internal
class TileIndexEntry
{
public:
    enum Status
    {
        MISSING_OR_STALE = 0,
        CHILD_TILE_PTR = 1,
        CURRENT = 2,
        CURRENT_WITH_MODIFIED = 3
    };

    explicit TileIndexEntry(uint32_t data) :  data_(data) {}
    TileIndexEntry(uint32_t page, Status status) :
        data_((page << 2) | status) {}

    uint32_t page() const { return data_ >> 2; }
    Status status() const { return static_cast<Status>(data_ & 3); }
    operator uint32_t() const { return data_; } // NOLINT implicit conversion
    bool isLoadedAndCurrent() const { return (data_ & 3) != 0; }

private:
    uint32_t data_;
};
// \endcond

} // namespace geodesk
