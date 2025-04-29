// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/store/BTreeData.h>
#include <cassert>
#include <cstring>

namespace clarisma {

template<typename Transaction, size_t MaxEntries, size_t MaxHeight>
class BTree : BTreeData
{
public:
    // TODO: should be based on page size
    static constexpr size_t MAX_ENTRIES = MaxEntries;
    static constexpr size_t MIN_ENTRIES = MAX_ENTRIES / 2;
    static constexpr size_t MAX_HEIGHT = MaxHeight;

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

        /// Remove the entry at logical position @p pos (0-based, not counting
        /// the header).  Does *not* rebalance the tree; the caller must take
        /// care of that if needed.
        ///
        /// @param pos  index of the entry to delete
        ///
        void remove(uint32_t pos)
        {
            assert(pos < count());

            if (pos < count() - 1)
            {
                std::memmove(&entries[pos + 1], &entries[pos + 2],
                    (count() - pos - 1) * sizeof(Entry));
            }
            count()--;
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

        void buildPathToLeftmost(Transaction* tx, PageNum page,
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
        }

        bool advance(Transaction* tx, uint32_t height)
        {
            Level& leaf = levels[height - 1];
            if (leaf.pos < leaf.node->count() - 1)
            {
                leaf.pos++;
                return true;
            }
            return advanceNode(tx, height);
        }

        bool advanceNode(Transaction* tx, uint32_t height)
        {
            // 2) Ascend until we can move right, then descend leftmost
            int depth = static_cast<int>(height) - 2;
            while (depth >= 0)
            {
                Level& level = levels[depth];
                Node* node = level.node;
                if (level.pos < node->count())
                {
                    level.pos++;
                    PageNum next = node->entries[level.pos].child;
                    buildPathToLeftmost(tx, next, depth + 1, height);
                    return true;
                }
                depth--;
            }
            // Reached end of root
            return false;
        }

        Level levels[MAX_HEIGHT];
    };

    class Iterator
    {
    public:
        Iterator(BTree* tree, Transaction* tx) :
            hasMore_(tree->height != 0),
            height_(tree->height),
            tx_(tx)
        {
            cursor_.buildPathToLeftmost(
                tx, tree->root, 0, tree->height);
        }

        bool hasMore() const { return hasMore_; }

        Entry& next()
        {
            assert(hasMore_);
            auto& leaf = cursor_.levels[height_-1];
            Entry& current = leaf.node->entries[leaf.pos+1];
            hasMore_ = cursor_.advance(tx_, height_);
            return current;
        }

    private:
        bool hasMore_;
        uint32_t height_;
        Transaction* tx_;
        Cursor cursor_;
    };

    // Entry& findLowerBound(Transaction* tx, uint32_t x) const;
    // Entry takeLowerBound(Transaction* tx, uint32_t x);

    const BTreeData& data() const { return *this; }
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

    static void freeNode(Transaction* tx, PageNum page)
    {
        tx->freeMetaPage(page);
    }

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

    // ==========================================================================
    //  Re-balancing helpers  (private: inside template<class …> class BTree)
    // ==========================================================================

    /// Small run-time check for the direction argument
    static constexpr bool isValidDir(int8_t d) noexcept
    {
        return d == -1 || d == +1;
    }

    /// Borrow one key (and its adjacent child pointer) from @p sibling and
    /// shift it into @p node.
    ///
    /// The caller must ***guarantee***
    ///   `sibling->count() > MIN_ENTRIES`  *and*  `isValidDir(direction)`.
    ///
    /// @param parent          parent node
    /// @param nodePos         index of @p node in the parent’s child list
    /// @param node            under-full node that *receives* the key
    /// @param sibling         sibling that *donates* the key
    /// @param direction       –1 → sibling is left of node, +1 → right
    ///
    static void borrowFromSibling(Node* parent, uint32_t nodePos,
        Node* node, Node* sibling, int8_t direction)
    {
        assert(isValidDir(direction));
        assert(sibling->count() > MIN_ENTRIES);

        const bool fromLeft  = (direction < 0);
        const bool fromRight = !fromLeft;

        // 1. Make room in @p node (only when borrowing from the left)
        if (fromLeft)
        {
            std::memmove(&node->entries[2],
                         &node->entries[1],
                         (node->count() + 1) * sizeof(Entry));
        }

        // 2. Copy key + child from @p sibling into @p node
        uint32_t srcPos    = fromLeft ? sibling->count()            // last entry
                                      : 1;                          // first entry
        uint32_t dstPos    = fromLeft ? 1                           // new first key
                                      : node->count() + 1;          // new last key
        node->entries[dstPos] = sibling->entries[srcPos];

        // 3. Update separator key in @p parent
        uint32_t sepKeyPos = fromLeft ? nodePos - 1 : nodePos;      // key index
        parent->entries[sepKeyPos + 1].key =
            fromLeft ? node->entries[1].key
                     : sibling->entries[2].key;    // new first key of right sib.

        // 4. Close the gap in @p sibling (only when borrowing from the right)
        if (fromRight)
        {
            std::memmove(&sibling->entries[1],
                         &sibling->entries[2],
                         (sibling->count() - 1) * sizeof(Entry));
        }

        node->count()++;
        sibling->count()--;
    }

    /// Merge @p node with its neighbouring @p sibling and delete the separator
    /// key from @p parent.  The *receiver* (the node that keeps the combined
    /// data) is chosen according to @p direction.
    ///
    /// Pre-conditions
    ///   * `isValidDir(direction)`
    ///   * `sibling` is exactly the child left (-1) or right (+1) of @p node
    ///
    /// @param tx             current transaction (needed to free the orphan page)
    /// @param parent         parent node
    /// @param nodePos        index of @p node in the parent’s child list
    /// @param node           under-full node
    /// @param sibling        adjacent sibling
    /// @param direction      –1 → merge with left sibling (left keeps data)
    ///                       +1 → merge with right sibling (node keeps data)
    ///
    void mergeWithSibling(Transaction* tx, Node* parent,
        uint32_t nodePos, Node* node, Node* sibling, int8_t direction)
    {
        assert(isValidDir(direction));

        const bool withLeft  = (direction < 0);
        Node* receiver = withLeft ? sibling : node;
        Node* donor    = withLeft ? node    : sibling;

        // 1. Bring separator key down into the receiver (interior only)
        if (!receiver->isLeaf())
        {
            uint32_t keyIdxParent = withLeft ? nodePos - 1 : nodePos;
            uint32_t dstPos       = receiver->count() + 1;

            receiver->entries[dstPos].key =
                parent->entries[keyIdxParent + 1].key;

            receiver->entries[dstPos].child = donor->entries[0].child;
            receiver->count()++;
        }

        // 2. Append all donor keys to the receiver
        std::memcpy(&receiver->entries[receiver->count() + 1],
                    &donor->entries[1],
                    donor->count() * sizeof(Entry));
        receiver->count() += donor->count();

        // 3. Remove separator key from parent
        uint32_t keyIdxParent = withLeft ? nodePos - 1 : nodePos;
        parent->remove(keyIdxParent);

        // 4. Free the now-empty donor page
        freeNode(tx, reinterpret_cast<PageNum>(donor));
    }

    /// Restore B-tree invariants after a single entry has been erased.
    ///
    /// Works its way up from the leaf stored in @p cursor, borrowing from or
    /// merging with siblings until every node holds at least `MIN_ENTRIES`
    /// keys, or until the root collapses.
    ///
    void rebalanceAfterErase(Transaction* tx, Cursor& cursor)
    {
        if (height == 0)
        {
            return;
        }

        // Walk from leaf (depth = height-1) toward the root (depth = 0)
        for (int depth = static_cast<int>(height) - 1; depth > 0; --depth)
        {
            typename Cursor::Level& leafLvl   = cursor.levels[depth];
            typename Cursor::Level& parentLvl = cursor.levels[depth - 1];

            Node* node       = leafLvl.node;
            Node* parent     = parentLvl.node;
            uint32_t nodePos = parentLvl.pos;          // child index in parent

            if (node->count() >= MIN_ENTRIES)
            {
                break;                  // All higher nodes are safe, we are done
            }

            // ----- 1.  try to borrow from left sibling -----------------------
            if (nodePos > 0)
            {
                Node* left = getNode(tx, parent->entries[nodePos - 1].child);
                if (left->count() > MIN_ENTRIES)
                {
                    borrowFromSibling(parent, nodePos, node, left, -1);
                    continue;           // balance restored for this level
                }
            }

            // ----- 2.  try to borrow from right sibling ----------------------
            if (nodePos < parent->count())
            {
                Node* right = getNode(tx, parent->entries[nodePos + 1].child);
                if (right->count() > MIN_ENTRIES)
                {
                    borrowFromSibling(parent, nodePos, node, right, +1);
                    continue;
                }
            }

            // ----- 3.  must merge – prefer left sibling when available -------
            if (nodePos > 0)
            {
                Node* left = getNode(tx, parent->entries[nodePos - 1].child);
                mergeWithSibling(tx, parent, nodePos, node, left, -1);

                // After merge the cursor now points at the surviving left node
                leafLvl.node = left;
                parentLvl.pos = nodePos - 1;
            }
            else
            {
                Node* right = getNode(tx, parent->entries[nodePos + 1].child);
                mergeWithSibling(tx, parent, nodePos, node, right, +1);
                // cursor already points at the survivor (node)
            }
            // Loop continues – parent may now be under-full
        }

        // ---------- 4.  root shrink check ------------------------------------
        Node* rootNode = getNode(tx, root);

        if (height > 1 && rootNode->count() == 0)
        {
            PageNum newRoot = rootNode->entries[0].child;
            freeNode(tx, root);
            root = newRoot;
            --height;
        }
        else if (height == 1 && rootNode->count() == 0)
        {
            freeNode(tx, root);
            root   = 0;
            height = 0;
        }
    }

public:
    size_t count(Transaction* tx) const
    {
        if (height == 0) return 0;
        size_t count = 0;
        Cursor cursor;
        cursor.buildPathToLeftmost(tx, root, 0, height);
        typename Cursor::Level& leaf = cursor.levels[height-1];
        do
        {
            count += leaf.node->count();
        }
        while (cursor.advanceNode(tx, height));
        return count;
    }

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
            bool insertLeft = pLevel->pos < (mid + isInterior);  // TODO: check
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

    /// Remove and return the first entry whose key is **≥ x**.
    ///
    /// @param tx  the active transaction
    /// @param x   lower-bound key
    /// @returns   the removed entry, or {0,0} if nothing matched
    ///
    Entry takeLowerBound(Transaction* tx, uint32_t x)
    {
        Cursor cursor;
        if (!findLowerBound(tx, x, cursor))
        {
            return {0, 0};     // nothing >= x
        }

        typename Cursor::Level& leafLevel = cursor.levels[height - 1];
        Node* leaf = leafLevel.node;
        uint32_t pos = leafLevel.pos;

        Entry removed = leaf->entries[pos + 1];
        leaf->remove(pos);

        rebalanceAfterErase(tx, cursor);      // see §3
        return removed;
    }

    /// Remove and return the first entry that matches both key and value.
    /// Multiple identical pairs may exist; only the first encountered is
    /// deleted.
    ///
    /// @param tx     the active transaction
    /// @param key    key to match
    /// @param value  value to match
    /// @returns      the removed entry, or {0,0} if not found
    ///
    Entry takeExact(Transaction* tx, uint32_t key, uint32_t value)
    {
        Cursor cursor;
        if (!findLowerBound(tx, key, cursor))
        {
            return {0, 0};
        }

        // Walk forward until we run out of duplicates or leaves
        do
        {
            typename Cursor::Level& leafLevel = cursor.levels[height - 1];
            Node* leaf = leafLevel.node;

            for (uint32_t p = leafLevel.pos; p < leaf->count(); ++p)
            {
                Entry& e = leaf->entries[p + 1];
                if (e.key != key)
                {
                    break;      // keys are ordered – no further matches
                }
                if (e.child == value)
                {
                    Entry removed = e;
                    leaf->remove(p);
                    rebalanceAfterErase(tx, cursor);
                    return removed;
                }
            }
        }
        while (cursor.advanceNode(tx, height));

        return {0, 0};          // not found
    }
};

} // namespace clarisma
