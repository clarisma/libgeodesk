// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/store/BTreeData.h>
#include <cstring>
#include <iterator>
#include <limits>

namespace clarisma {

template<typename Transaction>
class BTree : private BTreeData
{
public:
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
            uint32_t child;
        };
    };

    struct Node
    {
        uint32_t& count() { return entries[0].count; }
        bool isLeaf() const { return entries[0].child == 0; }

        void insert(int pos, Entry entry)
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

        Entry* buildPathToLeftmost(Transaction* tx, PageNum page,
            uint32_t depth, uint32_t height)
        {
            while (depth < height)
            {
                Node* node = BTree::getNode(tx, page);
                levels[depth].node = node;
                levels[depth].page = page;
                levels[depth].pos  = 0;
                page = node->entries[0].child;
                depth++;
            }
            return &levels[height-1].node->entries[1];
        }

        Entry* advance(Transaction* tx, uint32_t height)
        {
            // TODO: assert not already at end

            // 1) Move to next entry in current leaf if possible
            Level& leaf = levels[height - 1];
            if (leaf.pos + 1 <= leaf.node->count())
            {
                leaf.pos++;
                return &leaf.node->entries[leaf.pos];
            }

            // 2) Ascend until we can move right, then descend leftmost
            int depth = static_cast<int>(height) - 1;
            while (depth >= 0)
            {
                Level& level = levels[depth];
                Node* node = level.node;
                if (level.pos + 1 < node->count())
                {
                    level.pos++;
                    PageNum next = node->entries[level.pos].child;
                    return buildPathToLeftmost(tx, next, depth + 1, height);
                }
                depth--;
            }
            // Reached end of root
            return nullptr;
        }

        Level levels[MAX_HEIGHT];
    };

    class Iterator
    {
    public:
        Iterator(BTree* tree, Transaction* tx) :
            done_(tree->height == 0),
            height_(tree->height),
            tx_(tx)
        {
            if (!done_)
            {
                cursor_.buildPathToLeftmost(
                    tx, tree->root, 0, tree->height);
            }
        }

        Entry* next()
        {
            if (done_) return nullptr;
            Entry* current = cursor_.advance(tx_, height_);
            done_ = current==nullptr;
            return current;
        }

    private:
        bool done_;
        uint32_t height_;
        Transaction* tx_;
        Cursor cursor_;
    };

    // Entry& findLowerBound(Transaction* tx, uint32_t x) const;
    // Entry takeLowerBound(Transaction* tx, uint32_t x);

    Iterator iter(Transaction* tx) { return Iterator(this, tx); }

private:

    static Node* getNode(Transaction* tx, PageNum page)
    {
        return reinterpret_cast<Node*>(tx->getBlobBlock(page));
    }

    static PageNum allocNode(Transaction* tx)
    {
        return tx->allocMetaPage();
    }

    static void freeNode(Transaction* tx, PageNum page);

    /// Finds the path to the first (lowest) entry whose key is >= x
    ///
    /// @param tx       the transaction
    /// @param x        the key to be found
    /// @param cursor   the cursor where the path will be stored
    ///
    /// @returns true if an entry with key >= x has been found
    ///
    bool findLowerBound(Transaction* tx, uint32_t x, Cursor& cursor) const
    {
        typename Cursor::Level* level = &cursor.levels[0];
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

public:
    void insert(Transaction* tx, uint32_t key, uint32_t value)
    {
        Cursor cursor;
        Node* node;

        findLowerBound(tx, key, cursor);
        uint32_t level = height;
        typename Cursor::Level *pLevel = &cursor.levels[level];
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
};

} // namespace clarisma
