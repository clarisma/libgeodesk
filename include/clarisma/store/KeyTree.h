// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cassert>
#include <clarisma/util/log.h>

namespace clarisma {

// TODO: Check: Can an entry ever be inserted at first position in a node??
//  If so, its parent key would need to be updated
//  Safer to store that entry as last in left sibling node
//  Checked: Item will never be smaller than first item in leaf
//  We always descend into the leaf where the item should be found
//  if it existed

// TODO: mark modified nodes as dirty

template<
    typename Derived,
    size_t MAX_HEIGHT>
class KeyTree
{
public:
    using Key = uint64_t;
    using Pointer = uint64_t;
    static constexpr int ENTRY_SIZE_LEAF = 8;
    static constexpr int ENTRY_SIZE_INNER = 16;
    static constexpr int HEADER_SIZE = 8;

    explicit KeyTree(uint8_t* root) : root_(root) {}
    KeyTree()
    {
        root_ = self()->allocNode();
        Derived::initNode(root_, 8, true);
    }

    struct Level
    {
        uint8_t* node;
        int pos;
    };

    class Cursor
    {
    public:
        Cursor(Derived* tree) :
            tree_(tree),
            leaf_(nullptr) {}

        Cursor(const Cursor& other)
        {
            std::memcpy(this, &other, sizeof(Cursor));
            int n = other.leaf_ - &other.levels_[0];
            Level* adjustedLeaf = &levels_[n];
            leaf_ = other.leaf_ ? adjustedLeaf : nullptr;
        }

        Cursor& operator=(const Cursor& other)
        {
            std::memcpy(this, &other, sizeof(Cursor));
            int n = other.leaf_ - &other.levels_[0];
            Level* adjustedLeaf = &levels_[n];
            leaf_ = other.leaf_ ? adjustedLeaf : nullptr;
            return *this;
        }

        Level* leaf() { return leaf_; }

        int height() const
        {
            return leaf_ - &levels_[0] + 1;
        }

        Key key() const
        {
            return *reinterpret_cast<Key*>(leaf_->node + leaf_->pos * 8);
        }

        Key entry() const
        {
            return *entryPtr();
        }

        Key* entryPtr() const
        {
            return reinterpret_cast<Key*>(leaf_->node + leaf_->pos * 8);
        }

        bool isAfterLast() const
        {
            // Leaf entries are 1-based
            return leaf_->pos > Derived::leafKeyCount(leaf_->node);
        }

        bool isBeforeFirst() const
        {
            // Leaf entries are 1-based
            return leaf_->pos == 0;
        }

        void moveToLowerBound(Key key)
        {
            Level* level = &levels_[0];
            uint8_t* node = tree_->root();
            for(;;)
            {
                level->node = node;
                if(Derived::isLeaf(node))
                {
                    level->pos = findLeafNodeEntry(node, key);
                    leaf_ = level;
                    return;
                }
                level->pos = findInnerNodeEntry(node, key);
                node = getChildNode(level);
                ++level;

                // TODO: Check that maximum tree height is not exceeded
            }
        }

        bool moveToExact(Key k)
        {
            moveToLowerBound(k);
            return !isAfterLast() && key() == k;
        }

        void moveToFirst()
        {
            Level* level = &levels_[0];
            uint8_t* node = tree_->root();
            for(;;)
            {
                level->node = node;
                if(Derived::isLeaf(node))
                {
                    level->pos = 1;
                    leaf_ = level;
                    return;
                }
                level->pos = 0;
                node = getChildNode(level);
                ++level;

                // TODO: Check that maximum tree height is not exceeded
            }
        }

        void moveToLast()
        {
            Level* level = &levels_[0];
            uint8_t* node = tree_->root();
            for(;;)
            {
                level->node = node;
                if(Derived::isLeaf(node))
                {
                    level->pos = Derived::leafKeyCount(node);
                    leaf_ = level;
                    return;
                }
                level->pos = Derived::innerKeyCount(node)-1;
                node = getChildNode(level);
                ++level;

                // TODO: Check that maximum tree height is not exceeded
            }
        }

        void moveNext()
        {
            assert(!isAfterLast());
            Level* level = leaf_;
            uint8_t* node = level->node;
            if (++level->pos <= Derived::leafKeyCount(node)) return;
            while(level > &levels_[0])
            {
                --level;
                node = level->node;
                if (level->pos < Derived::innerKeyCount(node))
                {
                    ++level->pos;
                    for (;;)
                    {
                        node = getChildNode(level);
                        ++level;
                        assert(level < &levels_[MAX_HEIGHT]);
                        level->node = node;
                        if (Derived::isLeaf(node))
                        {
                            level->pos = 1;
                            leaf_ = level;
                            return;
                        }
                        level->pos = 0;
                    }
                }
            }
        }

        void movePrev()
        {
            assert(!isBeforeFirst());
            Level* level = leaf_;
            uint8_t* node = level->node;
            if (--level->pos > 0) return;
            while(level > &levels_[0])
            {
                --level;
                if (level->pos > 0)
                {
                    --level->pos;
                    for (;;)
                    {
                        node = getChildNode(level);
                        ++level;
                        assert(level < &levels_[MAX_HEIGHT]);
                        level->node = node;
                        if (Derived::isLeaf(node))
                        {
                            level->pos = Derived::leafKeyCount(node);
                            leaf_ = level;
                            return;
                        }
                        level->pos = Derived::innerKeyCount(node);
                    }
                }
            }
        }

        static uint8_t* getChildNode(Level* level)
        {
            return Derived::unwrapPointerAt(level->node + level->pos * 16 + 8);
        }

        /// @brief Returns a pointer to the child node value in
        /// the given inner node.
        ///
        /// @return A pointer to the child node pointer where `key`
        /// can be found (or would be inserted)
        ///
        /// This default implementation assumes internal nodes
        /// have this layout (all items are uint32_t):
        ///   payload_length (in bytes, excluding this header word)
        ///   leftmost_child
        ///   key0
        ///   child0
        ///   ...
        ///   key_n
        ///   child_n
        ///
        static int findInnerNodeEntry(uint8_t* node, Key key)
        {
            assert(!Derived::isLeaf(node));     // node must be an inner node
            size_t count = Derived::innerKeyCount(node);
            size_t left = 1;
            size_t right = count + 1;
            while (left < right)
            {
                size_t mid = left + (right - left) / 2;
                if (*reinterpret_cast<Key*>(node + mid * 16) <= key)
                {
                    left = mid + 1;
                }
                else
                {
                    right = mid;
                }
            }
            return left - 1;
        }

        /// @brief Returns a pointer to the value in the given
        /// leaf node whose key is the lowest key that is >= the
        /// search key; if all keys are lower, the returned pointer
        /// points beyond the entries (in that case, the address may
        /// not be valid if the node is full)
        ///
        /// This default implementation assumes internal nodes
        /// have this layout (all items are uint32_t):
        ///   payload_length (in bytes, excluding this header word)
        ///   always 0 (to distinguish from inner nodes)
        ///   key0
        ///   value0
        ///   ...
        ///   key_n
        ///   value_n
        ///
        static int findLeafNodeEntry(uint8_t* node, Key key)
        {
            assert(Derived::isLeaf(node));     // node must be leaf
            size_t count = Derived::leafKeyCount(node);
            size_t left = 1;
            size_t right = count + 1;
            while (left < right)
            {
                size_t mid = left + (right - left) / 2;
                if (*reinterpret_cast<Key*>(node + mid * 8) < key)
                {
                    left = mid + 1;
                }
                else
                {
                    right = mid;
                }
            }
            return left;
        }

        /// @brief Inserts the key and value. The cursor
        /// position is irrelevant prior
        /// to the call, and left indeterminate afterward.
        ///
        bool insert(Key k)
        {
            moveToLowerBound(k);
            if (!isAfterLast() && key() == k) return false;
            insertAtCurrent(k);
            return true;
        }

        /// @brief Inserts the key and value. The cursor
        /// position is irrelevant prior
        /// to the call, and left indeterminate afterward.
        ///
        void insertUnique(Key k)
        {
            moveToLowerBound(k);
            assert(isAfterLast() || key() != k);
            insertAtCurrent(k);
        }

        /// @brief Inserts an item after the current cursor
        /// position. The cursor may be located before the
        /// first item, but not be located after the last.
        /// The cursor position post-call is indeterminate.
        ///
        void insertAfterCurrent(Key key)
        {
            assert(!isAfterLast());
            ++leaf_->pos;
            insertAtCurrent(key);
        }

        void insertRaw(uint8_t* node, int pos, Key key, Pointer ptr)
        {
            bool isInternal = ptr != 0;
            assert(pos > 0);
            uint32_t entrySize = 8 << isInternal;
            uint32_t nodeSize = Derived::nodeSize(node);
            uint8_t* p = node + (pos << (3 + isInternal));
            uint8_t* end = node + nodeSize;
            memmove(p + entrySize, p, (end - p));
            *reinterpret_cast<Pointer*>(p + entrySize - 8) = ptr;
            *reinterpret_cast<Key*>(p) = key;
                // assumes key and pointer ar same type,
                // otherwise violates aliasing
            Derived::setNodeSize(node, nodeSize + entrySize);
        }

        // TODO: update parent key if insert at 0
        // TODO: masking of separator keys

        void insertAtCurrent(Key key)
        {
            Level* level = leaf_;
            assert(Derived::isLeaf(leaf_->node));
            Pointer ptr = 0;
            int maxNodeSize = Derived::maxNodeSize();
            bool isInternal = false;

            for (;;)
            {
                uint8_t* node = level->node;
                int pos = level->pos;
                uint32_t nodeSize = Derived::nodeSize(node);
                int entrySize = 8 << isInternal;
                if(nodeSize + entrySize <= maxNodeSize)
                {
                    // There's enough room in the node
                    insertRaw(node, pos, key, ptr);
                        // If ptr==0, we're inserting into a leaf
                    // TODO: if pos 0, update parent separator key
                    return;
                }
                // We need to split the node

                int numberOfKeys = keyCount(node, !isInternal);
                int keysInLeft = numberOfKeys / 2;
                uint8_t* rightNode = tree_->allocNode();

                // copy entries into the new right node
                // For leaf nodes, we copy everything starting
                // with the split key; for internal nodes, we skip
                // the split key, and instead copy its pointer value
                // into the slot for the leftmost pointer

                uint32_t leftSize = (keysInLeft + 1) << (3 + isInternal);
                uint8_t* p = node + leftSize;
                uint32_t skippedInternalKey = isInternal << 3;
                assert(skippedInternalKey == 0 || skippedInternalKey == sizeof(Key));
                uint8_t* src = p + skippedInternalKey;
                uint8_t* dest = rightNode + HEADER_SIZE;
                size_t bytesToCopy = ((numberOfKeys - keysInLeft) << (3 + isInternal))
                    - skippedInternalKey;
                uint32_t rightSize = bytesToCopy + HEADER_SIZE;
                Derived::initNode(rightNode, rightSize, !isInternal);
                std::memcpy(dest, src, bytesToCopy);
                Key splitKey = *reinterpret_cast<Key*>(p);

                // trim the left node
                Derived::setNodeSize(node, leftSize);

                // Now, insert the key & value
                bool insertRight = pos > keysInLeft + 1;
                insertRaw(insertRight ? rightNode : node,
                    insertRight ? (pos - keysInLeft - isInternal) : pos,
                    key, ptr);
                    // If inserting in the rightNode, we need to shift
                    // the position by 1 slot more if the node is an internal
                    // node, to account for the fact that the middle key
                    // is moved to the parent

                key = splitKey;
                ptr = Derived::wrapPointer(rightNode);
                isInternal = true;

                if (level == levels_) break;
                --level;
                ++level->pos;
            }

            // Create a new root level

            uint8_t* rootNode = tree_->allocNode();
            Derived::initNode(rootNode, 32, false);
            uint64_t* pWord = reinterpret_cast<uint64_t*>(rootNode);
            pWord[1] = Derived::wrapPointer(tree_->root());
            pWord[2] = key;
            pWord[3] = ptr;
            tree_->setRoot(rootNode);
        }

        // TODO: masking of separator keys
        // TODO: If removing first key in a leaf node,
        //  may need to update the parent node's separator key
        void remove()
        {
            auto minSize = tree_->minNodeSize();
            Level* level = leaf_;
            assert(Derived::isLeaf(leaf_->node));
            bool isInternal = false;

            for (;;)
            {
                uint8_t* node = level->node;
                int pos = level->pos;
                assert(pos > 0);
                printf("Deleting at pos %d\n", pos);
                fflush(stdout);

                auto nodeSize = Derived::nodeSize(node);
                uint32_t entrySize = 8 << isInternal;
                uint8_t* p = node + (pos << (3 + isInternal));
                uint8_t* end = node + nodeSize;
                uint8_t* pSrc = p + entrySize;
                std::memmove(p, pSrc, end - pSrc);
                nodeSize -= entrySize;
                Derived::setNodeSize(node, nodeSize);
                if (nodeSize >= minSize) return;

                if (level == levels())
                {
                    // We're at the root
                    assert(node == tree_->root());
                    if (isInternal && nodeSize == 16)
                    {
                        // If the root is an internal node and
                        // has one remaining pointer (the leftmost),
                        // delete this root and set the child as the new root
                        uint8_t* childNode = unwrapPointerAt(node + HEADER_SIZE);
                        tree_->freeNode(node);
                        tree_->setRoot(childNode);
                    }
                    return;
                }

                uint8_t* leftNode = nullptr;
                uint8_t* rightNode = nullptr;
                uint32_t leftSize;
                uint32_t rightSize;
                --level;
                uint8_t* parentNode = level->node;
                uint8_t* pParentSlot = parentNode + level->pos * 16;
                if (pParentSlot > parentNode)
                {
                    // child node has a left sibling
                    leftNode = Derived::unwrapPointerAt(pParentSlot-8);
                    leftSize = Derived::nodeSize(leftNode);
                    if (leftSize > minSize)
                    {
                        // can borrow from left sibling
                        Key borrowedKey = *reinterpret_cast<Key*>(
                            leftNode + leftSize - entrySize);
                        std::memmove(node + entrySize + HEADER_SIZE,
                            node + HEADER_SIZE, nodeSize - HEADER_SIZE);
                        if(isInternal)
                        {
                            // rightmost pointer of left sibling
                            // becomes the leftmost pointer of the
                            // current node
                            *reinterpret_cast<Pointer*>(node+8) =
                                *reinterpret_cast<Pointer*>(leftNode + leftSize-8);
                            // the separator key of the internal node's parent
                            // becomes the node's first key
                            *reinterpret_cast<Key*>(node+16) =
                                *reinterpret_cast<Key*>(pParentSlot);
                        }
                        else
                        {
                            *reinterpret_cast<Key*>(node+8) = borrowedKey;
                        }
                        // rightmost key of the left sibling
                        // becomes the parent's new separator key
                        *reinterpret_cast<Key*>(pParentSlot) = borrowedKey;

                        // Adjust node sizes
                        Derived::setNodeSize(node, nodeSize + entrySize);
                        Derived::setNodeSize(leftNode, leftSize - entrySize);
                        printf("Borrowed from left sibling\n");
                        fflush(stdout);
                        return;
                    }
                }

                auto parentSize = Derived::nodeSize(parentNode);
                if (pParentSlot < parentNode + parentSize - 16)
                {
                    // child node has a right sibling

                    rightNode = Derived::unwrapPointerAt(pParentSlot+24);
                    rightSize = Derived::nodeSize(rightNode);
                    if (rightSize > minSize)
                    {
                        // can borrow from right sibling
                        uint8_t* pDest = rightNode + entrySize;
                        Key borrowedKey = *reinterpret_cast<Key*>(pDest);

                        // Remember, here we need to work with the parent
                        // slot of the right node (not the parent slot
                        // of the current node), so pParentSlot+16
                        pParentSlot += ENTRY_SIZE_INNER;

                        if(isInternal)
                        {
                            // For an internal node, its parent's
                            // separator key becomes the node's new
                            // rightmost key
                            *reinterpret_cast<Key*>(node+nodeSize) =
                                *reinterpret_cast<Key*>(pParentSlot);
                            *reinterpret_cast<Pointer*>(node+nodeSize+sizeof(Key)) =
                                *reinterpret_cast<Pointer*>(rightNode + HEADER_SIZE);
                            // and the leftmost key of the right sibling
                            // becomes the parent's new separator key
                            *reinterpret_cast<Key*>(pParentSlot) = borrowedKey;
                        }
                        else
                        {
                            // For a leaf, the right sibling's leftmost key
                            // becomes the node's rightmost key
                            *reinterpret_cast<Key*>(node+nodeSize) = borrowedKey;

                            // Now we just set the parent's separator key
                            // to the new leftmost key of the right sibling
                            // (remember, we haven't shifted yet)
                            *reinterpret_cast<Key*>(pParentSlot) =
                                *reinterpret_cast<Key*>(rightNode + 16);
                        }

                        // adjust node sizes
                        Derived::setNodeSize(node, nodeSize + entrySize);
                        rightSize -= entrySize;
                        Derived::setNodeSize(rightNode, rightSize);
                        // Now, shift the right sibling's entries
                        // (remember, for internal nodes, we also shift the
                        // former first key's pointer into the position of
                        // the leftmost pointer)
                        // rightsize is already decreased to post-remove size
                        std::memmove(rightNode + HEADER_SIZE,
                            rightNode + entrySize + HEADER_SIZE,
                            rightSize - HEADER_SIZE);
                        printf("Borrowed from right sibling\n");
                        fflush(stdout);
                        return;
                    }
                }

                // merge nodes (right into left)

                if (leftNode)
                {
                    rightNode = node;
                    rightSize = nodeSize;
                    printf("Merging into left\n");
                    fflush(stdout);
                }
                else
                {
                    assert(rightNode);
                    leftNode = node;
                    leftSize = nodeSize;
                    ++level->pos;
                    pParentSlot += ENTRY_SIZE_INNER;   // This is a uint8_t pointer
                    printf("Merging into right\n");
                    fflush(stdout);
                }

                // for an internal node, the parent's separator key
                // migrated up into the point where the left and right
                // nodes are joined

                if (isInternal)
                {
                    *reinterpret_cast<Key*>(leftNode+leftSize) =
                        *reinterpret_cast<Key*>(pParentSlot);
                    leftSize += 8;
                        // because we insert the parent's separator key
                }
                memcpy(leftNode + leftSize,
                    rightNode + HEADER_SIZE, rightSize - HEADER_SIZE);
                Derived::setNodeSize(leftNode, leftSize + rightSize - HEADER_SIZE);
                assert(leftSize + rightSize - HEADER_SIZE <= tree_->maxNodeSize());
                tree_->freeNode(rightNode);
                isInternal = true;
            }
        }

        bool remove(Key k)
        {
            if (!moveToExact(k)) return false;
            remove();
            return true;
        }

        size_t size()
        {
            size_t count = 0;
            moveToFirst();
            while (!isAfterLast())
            {
                moveNext();
                count++;
            }
            return count;
        }

        Level* levels() { return levels_; }

    private:
        Derived* tree_;
        Level* leaf_;
        Level levels_[MAX_HEIGHT];
    };

    class Iterator
    {
    public:
        Iterator(Derived* tree) :
            cursor_(tree)
        {
            cursor_.moveToFirst();
        }

        bool hasNext() const { return !cursor_.isAfterLast(); }
        Key next()
        {
            Key k = cursor_.key();
            cursor_.moveNext();
            return k;
        }

    private:
        Cursor cursor_;
    };

    enum Error
    {
        OK,
        INVALID_NODE_SIZE,
        KEY_OUT_OF_RANGE,
        KEY_OUT_OF_ORDER,
        INVALID_NODE_REF,
        UNBALANCED
    };

    Key takeLowerBound(Key x)
    {
        Cursor cursor(self());
        cursor.moveToLowerBound(x);
        if (cursor.isAfterLast()) return Derived::nullEntry();
        Key k = cursor.key();
        cursor.remove();
        return k;
    }

protected:
    /// @brief Determines whether the given node is a leaf.
    ///
    static bool isLeaf(const uint8_t* node) // CRTP virtual
    {
        return *reinterpret_cast<const uint32_t*>(node + 4) == 1;
    }

    /// @brief Returns the actual size (including header) of
    /// the given node
    ///
    static size_t nodeSize(const uint8_t* node) // CRTP virtual
    {
        return *reinterpret_cast<const uint32_t*>(node) + 4;
    }

    static void setNodeSize(uint8_t* node, uint32_t size) // CRTP virtual
    {
        *reinterpret_cast<uint32_t*>(node) = size - 4;
    }

    static bool isEmpty(const uint8_t* node)    // CRTP virtual
    {
        return *reinterpret_cast<const uint32_t*>(node) == 8;
    }

    size_t maxNodeSize() const  // CRTP virtual
    {
        return 4096;
    }

    size_t minNodeSize() const
    {
        return self()->maxNodeSize() / 2 - 8;
            // TODO
    }

    static Key nullEntry()    // CRTP virtual
    {
        return 0;
    }

    /// @brief The number of keys in the given node
    ///
    static size_t keyCount(const uint8_t* node, bool isLeaf) // CRTP virtual
    {
        size_t count=  *reinterpret_cast<const uint32_t*>(node) >> (4 - isLeaf);
        assert(count == isLeaf ? leafKeyCount(node) : innerKeyCount(node));
        return count;
    }

    static void initNode(uint8_t* node, uint32_t nodeSize, bool isLeaf)
    {
        *reinterpret_cast<uint32_t*>(node) = nodeSize - 4;
        *reinterpret_cast<uint32_t*>(node + 4) = isLeaf;
    }

    /// @brief Returns the number of entries in an inner node
    ///
    static size_t innerKeyCount(const uint8_t* node)  // CRTP virtual
    {
        return Derived::nodeSize(node) / 16 - 1;
    }

    /// @brief Returns the number of entries in a leaf node
    ///
    static size_t leafKeyCount(const uint8_t* node)  // CRTP virtual
    {
        return Derived::nodeSize(node) / 8 - 1;
    }

    Derived* self() { return static_cast<Derived*>(this); }
    const Derived* self() const { return static_cast<const Derived*>(this); }

    uint8_t* allocNode();             // CRTP virtual
    void freeNode(uint8_t* node);     // CRTP virtual

    static uint8_t* unwrapPointerAt(uint8_t* p)    // CRTP virtual
    {
        Pointer node = *reinterpret_cast<Pointer*>(p);
        return reinterpret_cast<uint8_t*>(node);
    }

    static Pointer wrapPointer(uint8_t* node)    // CRTP virtual
    {
        return reinterpret_cast<Pointer>(node);
    }

    void checkNode(uint8_t* node, uint64_t min, uint64_t maxExclusive)
    {
        bool isLeaf = Derived::isLeaf(node);
        uint64_t prev = min==0 ? min : min-1;
        uint8_t* p = node + 8;
        uint8_t* end = node + Derived::nodeSize(node);
        if (!isLeaf)
        {
            uint8_t* childNode = Derived::unwrapPointerAt(p);
            checkNode(childNode, min, *reinterpret_cast<uint64_t*>(p + 16));
            p += 8;
        }
        while(p < end)
        {
            uint64_t k = *reinterpret_cast<uint64_t*>(p);
            assert(k > prev);
            assert(k < maxExclusive);
            if (!isLeaf)
            {
                p += 8;
                uint8_t* childNode = Derived::unwrapPointerAt(p);
                uint8_t* next = p + 16;
                checkNode(childNode, k, next < end ?
                    *reinterpret_cast<uint64_t*>(next) : maxExclusive);
            }
            prev = k;
            p += 8;
        }
    }

public:
    uint8_t* root() const { return root_; }
    void setRoot(uint8_t* root) { root_ = root; }

    Iterator iter()
    {
        return Iterator(self());
    }

    bool insert(Key k)
    {
        Cursor cursor(self());
        return cursor.insert(k);
    }

    void check()
    {
        checkNode(root_, 0, std::numeric_limits<uint64_t>::max());
    }

    size_t size()
    {
        size_t count = 0;
        auto it = iter();
        while (it.hasNext())
        {
            it.next();
            count++;
        }
        return count;
    }


private:
    uint8_t* root_;
};

} // namespace clarisma
