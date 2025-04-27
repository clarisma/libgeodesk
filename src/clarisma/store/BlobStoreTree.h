// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/store/BlobStore.h>

namespace clarisma {

class BlobStoreTree
{
private:
    using PageNum = BlobStore::PageNum;

    // TODO: should be based on page size
    static constexpr uint32_t MAX_ENTRIES = 511;
    static constexpr uint32_t MIN_ENTRIES = MAX_ENTRIES / 2 - 1;


    struct Entry
    {
        union
        {
            uint32_t key;
            uint32_t count;
        };
        union
        {
            uint32_t value;
            PageNum child;
        };
    };

    struct Node
    {

};

} // namespace clarisma
