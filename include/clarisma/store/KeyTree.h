// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cassert>
#include <cstring>
#include <clarisma/util/log.h>

namespace clarisma {

// TODO: Check: Can an entry ever be inserted at first position in a node??
//  If so, its parent key would need to be updated
//  Safer to store that entry as last in left sibling node
//  Checked: Item will never be smaller than first item in leaf
//  We always descend into the leaf where the item should be found
//  if it existed

template<
    typename Derived,
    size_t MAX_HEIGHT>
class BTree
{
public:
    using Key = uint64_t;
    using Pointer = uint64_t;

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

        Transaction* transaction() const { return transaction_; };

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

        NodeRef root() const { return *root_; }
        void setRoot(NodeRef root) const { *root_ = root; }

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
            uint8_t* node = getNode(*root_);
            for(;;)
            {
                level->node = node;
                if(Derived::isLeaf(node))
                {
                    level->pos = Derived::findLeafNodeEntry(node, key);
                    leaf_ = level;
                    return;
                }
                level->pos = Derived::findInnerNodeEntry(node, key);
                node = Derived::getChildNode(transaction_, level);
                ++level;

                // TODO: Check that maximum tree height is not exceeded
            }
        }

        void moveToFirst()
        {
            Level* level = &levels_[0];
            uint8_t* node = getNode(*root_);
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
                node = Derived::getChildNode(transaction_, level);
                ++level;

                // TODO: Check that maximum tree height is not exceeded
            }
        }

        void moveToLast()
        {
            Level* level = &levels_[0];
            uint8_t* node = getNode(*root_);
            for(;;)
            {
                level->node = node;
                if(Derived::isLeaf(node))
                {
                    level->pos = Derived::leafKeyCount(node);
                    leaf_ = level;
                    return;
                }
                level->pos = Derived::innerleafKeyCount(node)-1;
                node = Derived::getChildNode(transaction_, level);
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
                        node = Derived::getChildNode(transaction_, level);
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
                        node = Derived::getChildNode(transaction_, level);
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
            return Derived::unwrapPointerAt(level->node + level->pos * 16 + 4);
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
            size_t count = Derived::innerNodeEntryCount(node);
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
            size_t count = Derived::leafNodeEntryCount(node);
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
        void insert(Key key)
        {
            moveToLowerBound(key);
            insertAtCurrent(key);
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
            return;
        }

        // TODO: update parent key if insert at 0
        // TODO: masking of separator keys

        void insertAtCurrent(Key key, Key keyMask)
        {
            Level* level = leaf_;
            assert(Derived::isLeaf(leaf_));
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
                    insertRaw(p, pos, key, ptr);
                        // If ptr==0, we're inserting into a leaf
                    // TODO: if pos 0, update parrent separator key
                    return;
                }
                // We need to split the node

                int numberOfKeys = keyCount(node, isLeaf);
                int splitPos = numberOfKeys / 2;
                uint8_t* rightNode = Derived::allocNode();

                // copy entries into the new right node
                // For leaf nodes, we copy everything starting
                // with the split key; for internal nodes, we skip
                // the split key, and instead copy its pointer value
                // into the slot for the leftmost pointer

                uint8_t* p = node + (splitPos << (3 + isInternal));
                uint32_t extraForInternal = isInternal << 3;
                uint8_t* src = p + extraForInternal;
                uint8_t* dest = rightNode + 8;
                size_t bytesToCopy = ((numberOfKeys - splitPos) << (3 + isInternal))
                    + extraForInternal;
                uint32_t rightNodeSize = bytesToCopy + 8;
                Derived::initNode(rightNodeSize, !isInternal);
                std::memcpy(dest, src, bytesToCopy);
                Key splitKey = *reinterpret_cast<Key*>(p);

                // trim the left node
                Derived::setNodeSize(node, splitPos << (3 + isInternal));

                // Now, insert the key & value
                bool insertRight = pos > splitPos + 1;
                insertRaw(insertRight ? rightNode : node,
                    insertRight ? (pos - splitPos - isInternal) : pos,
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

            uint8_t* rootNode = Derived::allocNode();
            Derived::initNode(rootNode, false);
            uint64_t* pWord = reinterpret_cast<uint64_t*>(rootNode);
            pWord[1] = Derived::wrapPointer(tree_->root());
            pWord[2] = key;
            pWord[3] = Derived::wrapPointer(ptr);
            tree_->setRoot(rootNode);
        }

        // TODO: masking of separator keys
        // TODO: If removing first key in a leaf node,
        //  may need to update the parent node's separator key
        void remove()
        {
            auto minSize = Derived::minNodeSize(tx);
            Level* level = leaf_;
            assert(Derived::isLeaf(leaf_));
            bool isInternal = false;

            for (;;)
            {
                uint8_t* node = level->node;
                int pos = level->pos;
                auto nodeSize = Derived::nodeSize(node);
                uint32_t entrySize = 8 << isInternal;
                uint8_t* p = node + (pos << (3 + isInternal));
                uint8_t* end = node + nodeSize;
                uint8_t* pSrc = p + entrySize;
                std::memmove(p, pSrc, end - pSrc);
                nodeSize -= entrySize;
                Derived::setNodeSize(node, nodeSize);
                if (nodeSize >= minSize) return;

                if (level == cursor.levels())
                {
                    // We're at the root

                    if (isInternal && nodeSize == 16)
                    {
                        // If the root is an internal node and
                        // has one remaining pointer (the leftmost),
                        // delete this root and set the child as the new root
                        Pointer childNode = *reinterpret_cast<Pointer*>(node + 8);
                        Derived::freeNode(childNode);
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
                uint8_t* pParentSlot = parentNode + level->pos * 16);
                if (pParentSlot > node)
                {
                    // child node has a left sibling
                    leftNode = Derived::unwrapPointerAt(pParentSlot-8));
                    leftSize = Derived::nodeSize(leftNode);
                    if (leftSize > minSize)
                    {
                        // can borrow from left sibling
                        Key borrowedKey = *reinterpret_cast<Key*>(
                            leftNode + leftSize - entrySize);
                        std::memmove(node + entrySize * 2, node + entrySize,
                            nodeSize - entrySize);
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
                                *(pParentSlot-1);
                        }
                        else
                        {
                            *reinterpret_cast<Key*>(node+8) = borrowedKey;
                        }
                        // rightmost key of the left sibling
                        // becomes the parent's new sparator key
                        *reinterpret_cast<Key*>(pParentSlot) = borrowedKey;

                        // Adjust node sizes
                        Derived::setNodeSize(node, nodeSize + entrySize);
                        Derived::setNodeSize(leftNode, leftSize - entrySize);
                        return;
                    }
                }

                auto parentSize = Derived::nodeSize(parentNode);
                if (pParentSlot < parentNode + parentSize - 16)
                {
                    // child node has a right sibling

                    rightNode = Derived::unwrapPointerAt(pParentSlot+8));
                    rightSize = Derived::nodeSize(rightNode);
                    if (rightSize > minSize)
                    {
                        // can borrow from right sibling
                        uint8_t* pDest = rightNode + entrySize;
                        Key borrowedKey = *reinterpret_cast<Key*>(pDest);
                        // For both leaf and internal, the right sibling's
                        // key or leftmost pointer becomes the node's
                        // rightmost element
                        reinterpret_cast<Pointer*>(node+nodeSize-8) =
                            reinterpret_cast<Pointer*>(rightNode + 8);

                        if(isInternal)
                        {
                            // For an internal node, its parent's
                            // separator key becomes the node's new
                            // rightmost key
                            reinterpret_cast<Key*>(node+nodeSize-16) =
                                reinterpret_cast<Key*>(pParentSlot);
                            // and the leftmost key of the right sibling
                            // becomes the parent's new separator key
                            *reinterpret_cast<Key*>(pParentSlot) = borrowedKey;
                        }
                        else
                        {
                            // For a leaf, we've already copied the right
                            // sibling's leftmost key to the end of the node
                            // Now we just set the parent's separator key
                            // to the new leftmost key of the right sibling
                            // (remember, we haven't shifted yet)
                            *reinterpret_cast<Key*>(pParentSlot) =
                                reinterpret_cast<Pointer*>(rightNode + 16);
                        }

                        // adjust node sizes
                        Derived::setNodeSize(node, nodeSize + entrySize);
                        rightSize -= entrySize;
                        Derived::setNodeSize(rightNode, rightSize);
                        // Now, shift the right sibling's entries
                        // (remember, for internal nodes, we also shift the
                        // former first key's pointer into the position of
                        // the leftmost pointer)
                        std::memmove(rightNode + 8, rightNode + entrySize * 2,
                            rightSize - 8);
                        return;
                    }
                }

                // merge nodes (right into left)

                uint8_t* nodeToDelete;
                if (leftNode)
                {
                    rightNode = node;
                    rightSize = nodeSize;
                    nodeToDelete = node;
                }
                else
                {
                    assert(rightNode);
                    leftNode = node;
                    leftSize = nodeSize;
                    ++level->pos;
                    pParentSlot += 16;        // This is a uint8_t pointer
                    nodeToDelete = rightNode;
                }

                // for an internal node, the parent's separator key
                // migrated up into the point where the left and right
                // nodes are joined

                if (isInternal)
                {
                    *reinterpret_cast<Key*>(leftNode+leftSize) =
                        reinterpret_cast<Key*>(pParentSlot);
                    leftSize += 8;
                }
                memcpy(leftNode + leftSize + (isInternal << 3),
                    rightNode+8, rightSize-8);
                setNodeSize(leftNode, leftSize + rightSize - 8);
                Derived::freeNode(nodeToDelete);
                isInternal = true;
            }
        }

        bool remove(Key k, Value v)
        {
            if (!moveToExact(k,v)) return false;
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
        uint8_t* getNode(NodeRef ref)
        {
            return Derived::getNode(transaction_, ref);
        }

        Derived* tree_;
        Level* leaf_;
        Level levels_[MaxHeight];
    };

    class Iterator
    {
    public:
        Iterator(Transaction* tx, NodeRef* root) :
            cursor_(tx, root)
        {
            cursor_.moveToFirst();
        }

        bool hasNext() const { return !cursor_.isAfterLast(); }
        Entry next()
        {
            Entry entry = cursor_.entry();
            cursor_.moveNext();
            return entry;
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

    static Iterator iter(Transaction* tx, Value* root)
    {
        return Iterator(tx, root);
    }

    static Entry takeLowerBound(Transaction* tx, NodeRef* root, Key x)
    {
        Cursor cursor(tx, root);
        cursor.moveToLowerBound(x);
        if (cursor.isAfterLast()) return Derived::nullEntry();
        auto e = cursor.entry();
        remove(cursor);
        return e;
    }

protected:
    /// @brief Determines whether the given node is a leaf.
    ///
    static bool isLeaf(const uint8_t* node) // CRTP virtual
    {
        return *reinterpret_cast<const uint32_t*>(node + 4) == 0;
    }

    /// @brief Returns the actual size (including header) of
    /// the given node
    ///
    static size_t nodeSize(const uint8_t* node) // CRTP virtual
    {
        return *reinterpret_cast<const uint32_t*>(node) + 4;
    }

    static bool isEmpty(const uint8_t* node)    // CRTP virtual
    {
        return *reinterpret_cast<const uint32_t*>(node) == 8;
    }

    size_t maxNodeSize()  // CRTP virtual
    {
        return 4096;
    }

    static size_t minNodeSize(Transaction* tx)
    {
        return Derived::maxNodeSize(tx) / 2 - 8;
    }

    static Entry nullEntry()    // CRTP virtual
    {
        return {0,0};
    }

    /// @brief The number of keys in the given node
    ///
    static size_t keyCount(const uint8_t* node, bool isLeaf) // CRTP virtual
    {
        return *reinterpret_cast<const uint32_t*>(node) >> (4 - isLeaf);
    }

    static void initNode(uint8_t* node, uint32_t nodeSize, bool isLeaf)
    {
        *reinterpret_cast<uint64_t*>(node) =
            static_cast<uint64_t>(nodeSize - 4) |
                (isInternal ? 0 0x1'0000'0000ULL);
            // Set node size and leaf-node flag
    }

    /// @brief Returns the number of entries in an inner node
    ///
    static size_t innerNodeKeyCount(const uint8_t* node)  // CRTP virtual
    {
        return Derived::nodeSize(node) / 16 - 1;
    }

    /// @brief Returns the number of entries in a leaf node
    ///
    static size_t leafNodeKeyCount(const uint8_t* node)  // CRTP virtual
    {
        return Derived::nodeSize(node) / 8 - 1;
    }

    /// Returns a pointer to a node, based on the given node reference
    /// (e.g. the number of the page that holds the node for a disk-based
    /// implementation)
    ///
    static uint8_t* getNode(Transaction* tx, NodeRef ref);    // CRTP virtual

    static std::pair<NodeRef,uint8_t*> allocNode(Transaction* tx);     // CRTP virtual

    static void freeNode(Transaction* tx, NodeRef ref);     // CRTP virtual



    /// @brief Inserts a key/value pair into a node. This method
    /// assumes that the node has sufficient room.
    ///
    static void insertRaw(uint8_t* node, int pos, Key key, Value value)
    {
        uint8_t* p = Derived::ptrOfEntry(node, pos);
        uint8_t* end = node + Derived::nodeSize(node);
        if(p < end)
        {
            std::memmove(p + 8, p, end - p);
        }
        *reinterpret_cast<uint32_t*>(p) = static_cast<uint32_t>(key);
        *reinterpret_cast<uint32_t*>(p+4) = static_cast<uint32_t>(value);
        *reinterpret_cast<uint32_t*>(node) += 8;
    }


public:
    uint8_t* root() const { return root_; }
    void setRoot(uint8_t* root) { root_ = root; }

    /*
    static void init(Transaction* tx, NodeRef* root)
    {
        auto [ref, node] = Derived::allocNode(tx);
        *reinterpret_cast<uint64_t*>(node) = 4;
        *root = ref;
    }
    */

    /*
    Error verifyNode(Transaction* tx, uint8_t* node, Key minKey, Key maxKey)
    {

    }
    */


private:
    uint8_t* root_;
};

} // namespace clarisma
