// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/store/BlobStore.h>

namespace clarisma {

struct BlobStoreTree
{
private:
    using PageNum = BlobStore::PageNum;

    // TODO: should be based on page size
    static constexpr size_t MAX_ENTRIES = 511;
    static constexpr size_t MIN_ENTRIES = (MAX_ENTRIES + 1) / 2;
    static constexpr size_t MAX_HEIGHT = 8;

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
        uint32_t& count() { return entries[0].count; }
        bool isLeaf() const { return entries[0].child == 0; }
        void insert(int pos, Entry entry);

        Entry entries[MAX_ENTRIES + 1];
    };

    struct Cursor
    {
        struct Level
        {
            Node* node;
            PageNum page;
            uint32_t pos;
        };

        Level levels[MAX_HEIGHT];
    };

    static Node* getNode(BlobStore::Transaction* tx, PageNum page);
    static PageNum allocNode(BlobStore::Transaction* tx);
    static void freeNode(BlobStore::Transaction* tx, PageNum page);
    bool findLowerBound(BlobStore::Transaction* tx, uint32_t x, Cursor& cursor) const;
    void insert(BlobStore::Transaction* tx, uint32_t key, uint32_t value);

    PageNum root = 0;
    uint32_t height = 0;
};

} // namespace clarisma
