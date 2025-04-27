// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/store/BlobStoreTree.h>
#include <cstring>

namespace clarisma {

/*
BlobStoreTree::Entry& BlobStoreTree::findLowerBound(Transaction* tx, uint32_t x) const
{

}
*/

BlobStoreTree::Node* BlobStoreTree::getNode(Transaction* tx, PageNum page)
{
    return reinterpret_cast<Node*>(tx->getBlobBlock(page));
}

/// Finds the path to the first (lowest) entry whose key is >= x
///
/// @param tx       the transaction
/// @param x        the key to be found
/// @param cursor   the cursor where the path will be stored
///
/// @returns true if an entry with key >= x has been found
///
bool BlobStoreTree::findLowerBound(Transaction* tx,
    uint32_t x, Cursor& cursor) const
{
    Cursor::Level* level = &cursor.levels[0];
    Node* node;

    if(!root) return false;
        // TODO: nonzero root be precondition instead?

    // 1) Descend to find lower_bound
    PageNum page = root;
    uint32_t lo = 0;
    do
    {
        node = getNode(tx, page);

        // binary search for first entry.key >= x
        lo = 0;
        uint32_t hi = node->count();
        while (lo < hi)
        {
            uint32_t mid = (lo + hi) >> 1;
            if (node->entries[mid+1].key < x)   // entries are 1-based due to header
            {
                lo = mid + 1;
            }
            else
            {
                hi = mid;
            }
        }

        level->node = node;
        level->page = page;
        level->pos = lo;
        level++;

        assert(level - &cursor.levels[0] < MAX_HEIGHT);

        if (node->isLeaf()) break;
        page = node->entries[lo].child;
    }
    while (page);
    assert(level - &cursor.levels[0] == height);
        // TODO: could base loop on level, which
        //  would free up the child ptr of leaf node
        //  (possible use: chaining leaves)

    return lo < node->count();
}


void BlobStoreTree::Node::insert(int pos, Entry entry)
{
    assert(count() < MAX_ENTRIES);
    if (pos < count())
    {
        std::memmove(&entries[pos+2], &entries[pos+1],
        (count() - pos) * sizeof(Entry));
    }
    entries[pos+1] = entry;
    // Remember, entries[0] is the header!
    count()++;
}

void BlobStoreTree::insert(Transaction* tx, uint32_t key, uint32_t value)
{
    Cursor cursor;
    Node* node;

    findLowerBound(tx, key, cursor);
    uint32_t level = height;
    Cursor::Level *pLevel = &cursor.levels[level];
    bool isInterior = false;
    while (level > 0)
    {
        level--;
        pLevel--;
        node = pLevel->node;
        assert(node->count() <= MAX_ENTRIES);
        assert(node->count() >0 || level==0);
        if (node->count() < MAX_ENTRIES)
        {
            // no split required
            node->insert(pLevel->pos, {key,value});
            return;
        }
        uint32_t mid = node->count() / 2;
        uint32_t parentKey = node->entries[mid+1].key;
            // entry 0 is the header
        PageNum newNodePage = allocNode(tx);
        Node* newNode = getNode(tx, newNodePage);
        uint32_t newCount = node->count() - mid - isInterior;
        newNode->count() = newCount;
        newNode->entries[0].child = isInterior ? node->entries[mid+1].child : 0;
        std::memcpy(&newNode->entries[1], &node->entries[mid+ 1 +isInterior],
            newCount * sizeof(Entry));
            // entry 0 is the header
        node->count() = mid;     // trim left node
        bool insertLeft = pLevel->pos < mid;  // TODO: check
        Node* targetNode = insertLeft ? node : newNode;
        uint32_t targetPos = insertLeft ? pLevel->pos : pLevel->pos - mid - isInterior;
        targetNode->insert(targetPos, {key,value});
        isInterior = true;
        key = parentKey;
        value = newNodePage;
    }
    // create new root
    height++;
    PageNum oldRoot = root;
    root = allocNode(tx);
    node = getNode(tx, root);
    node->count() = 1;
    node->entries[0].child = oldRoot;
    node->entries[1].key = key;
    node->entries[1].child = value;
}

} // namespace clarisma
