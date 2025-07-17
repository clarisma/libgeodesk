// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/store/BTreeData.h>
#include <cassert>
#include <cstring>
#include <clarisma/util/log.h>

namespace clarisma {

// TODO: Fix copy construction of Cursor, leaf_ must be adjusted!

template<typename Derived, typename Transaction, size_t MaxHeight>
class BTree
{
public:
    using Key = uint64_t;
    using Value = uint32_t;

    struct Level
    {
        uint8_t* node;
        int pos;
        // Value value;
    };

    class Cursor
    {
    public:
        Cursor(Transaction* tx, Value* root) :
            transaction_(tx), root_(root), leaf_(nullptr) {}

        Cursor(const Cursor& other)
        {
            std::memcpy(this, &other, sizeof(Cursor));
            int n = other.leaf_ - &other.levels_[0];
            Level* adjustedLeaf = &levels_[n];
            leaf_ = leaf_ ? adjustedLeaf : nullptr;
        }

        Cursor& operator=(const Cursor& other)
        {
            std::memcpy(this, &other, sizeof(Cursor));
            int n = other.leaf_ - &other.levels_[0];
            Level* adjustedLeaf = &levels_[n];
            leaf_ = leaf_ ? adjustedLeaf : nullptr;
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
            return *reinterpret_cast<uint32_t*>(leaf_->node + leaf_->pos * 8);
        }

        Value value() const
        {
            return *reinterpret_cast<Value*>(leaf_->node + leaf_->pos * 8 + 4);
        }

        std::pair<Key,Value> entry() const
        {
            return { key(), value() };
        }

        Value root() const { return *root_; }
        void setRoot(Value root) const { *root_ = root; }

        bool isAfter() const
        {
            return leaf_->pos > Derived::keyCount(leaf_->node);
        }

        void findLowerBound(Key key)
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
                int count = Derived::keyCount(node);
                if(Derived::isLeaf(node))
                {
                    level->pos = count;
                    leaf_ = level;
                    return;
                }
                level->pos = count-1;
                node = Derived::getChildNode(transaction_, level);
                ++level;

                // TODO: Check that maximum tree height is not exceeded
            }
        }

        void moveNext()
        {
            assert(!isAfter());
            Level* level = leaf_;
            uint8_t* node = level->node;
            if (++level->pos <= Derived::keyCount(node)) return;
            while(level > &levels_[0])
            {
                --level;
                node = level->node;
                if (level->pos < Derived::keyCount(node))
                {
                    ++level->pos;
                    for (;;)
                    {
                        node = Derived::getChildNode(transaction_, level);
                        ++level;
                        assert(level < &levels_[MaxHeight]);
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

        Level* levels() { return levels_; }

    private:
        uint8_t* getNode(Value ref)
        {
            return Derived::getNode(transaction_, ref);
        }

        Transaction* transaction_;
        Value* root_;
        Level* leaf_;
        Level levels_[MaxHeight];
    };

    class Iterator
    {
    public:
        Iterator(Transaction* tx, Value* root) :
            cursor_(tx, root)
        {
            cursor_.moveToFirst();
        }

        bool hasNext() const { return !cursor_.isAfter(); }
        std::pair<Key,Value> next()
        {
            std::pair<Key,Value> entry = { cursor_.key(), cursor_.value() };
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

    Iterator iter(Transaction* tx, Value* root)
    {
        return Iterator(tx, root);
    }

    std::pair<Key,Value> takeLowerBound(Transaction* tx, Value* root, Key x)
    {
        Cursor cursor(tx, root);
        cursor.findLowerBound(x);
        if (cursor.isAfter()) return {0,0};
        auto e = cursor.entry();
        remove(cursor);
        return e;
    }

protected:
    /// @brief Determines whether the given node is a leaf.
    ///
    static bool isLeaf(const uint8_t* node)
    {
        return *reinterpret_cast<const uint32_t*>(node + 4) == 0;
    }

    /// @brief Returns the actual size (including header) of
    /// the given node
    ///
    static size_t nodeSize(const uint8_t* node)
    {
        return *reinterpret_cast<const uint32_t*>(node) + 4;
    }

    static bool isEmpty(const uint8_t* node)
    {
        return *reinterpret_cast<const uint32_t*>(node) == 8;
    }

    static size_t maxNodeSize(Transaction* tx)
    {
        return 4096;
    }

    static size_t minNodeSize(Transaction* tx)
    {
        return Derived::maxNodeSize(tx) / 2 - 8;
    }

    /// @brief The number of keys in the given node
    ///
    static size_t keyCount(const uint8_t* node)
    {
        return *reinterpret_cast<const uint32_t*>(node) / 8;
    }

    /// @brief The pointer to the given entry
    ///
    static uint8_t* ptrOfEntry(uint8_t* node, int pos)
    {
        return node + pos * 8;
    }

    /// @brief Returns the number of entries in an inner node
    ///
    static size_t innerNodeEntryCount(const uint8_t* node)
    {
        return Derived::nodeSize(node) / 8 - 1;
    }

    /// @brief Returns the number of entries in a leaf node
    ///
    static size_t leafNodeEntryCount(const uint8_t* node)
    {
        return Derived::nodeSize(node) / 8 - 1;
    }

    /// Returns a pointer to a node, based on the given node reference
    /// (e.g. the number of the page that holds the node for a disk-based
    /// implementation)
    ///
    static uint8_t* getNode(Transaction* tx, Value ref);    // CRTP override

    static std::pair<Value,uint8_t*> allocNode(Transaction* tx);     // CRTP override

    static std::pair<Value,uint8_t*> freeNode(Transaction* tx, Value ref);     // CRTP override

    /*
    static uint8_t* createNode()
    {
        uint8_t* node = Derived::allocNode();
        *reinterpret_cast<uint64_t*>(node) = 4;
        // Set payload size to 4 and mark as leaf
        return node;
    }
    */

    static uint8_t* getChildNode(Transaction* tx, Level* level)
    {
        return Derived::getNode(tx, *reinterpret_cast<Value*>(
            level->node + level->pos * 8 + 4));
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
            if (*reinterpret_cast<uint32_t*>(node + mid * 8) <= key)
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
            if (*reinterpret_cast<uint32_t*>(node + mid * 8) < key)
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


    // static bool tryInsert(

    static void insert(Transaction* tx, Value* root, Key key, Value value)
    {
        Cursor cursor(tx, root);
        cursor.findLowerBound(key);
        Level* level = cursor.leaf();
        for (;;)
        {
            uint8_t* node = level->node;
            int pos = level->pos;
            if(nodeSize(node) + 8 <= Derived::maxNodeSize(tx))
            {
                // There's enough room in the node
                insertRaw(node, pos, key, value);
                return;
            }
            // We need to split the node
            auto [rightRef, rightNode] = Derived::allocNode(tx);
            bool leafFlag = isLeaf(node);
            int numberOfKeys = keyCount(node);
            int splitPos = numberOfKeys / 2;

            if(!leafFlag)
            {
                // std::cout << "Splitting internal node.";
            }

            // copy entries into the new right node
            // For leaf nodes, we copy everything starting
            // with the split key; for internal nodes, we skip
            // the split key, and instead copy its pointer value
            // into the slot for the leftmost pointer
            // (so for internals, we copy 4 bytes less, and place
            // them 4 bytes earlier into the node)

            uint8_t* src = node + splitPos * 8 + (leafFlag ? 8 : 12);
            uint8_t* dest = rightNode + 4 + leafFlag * 4;
            size_t rightPayloadSize = (numberOfKeys - splitPos - !leafFlag) * 8 + 4;
            size_t bytesToCopy = rightPayloadSize - (leafFlag ? 4 : 0);
            *reinterpret_cast<uint64_t*>(rightNode) = rightPayloadSize;
                // This sets word 1 to 0, indicating a leaf
                // copying an internal node will overwrite this with
                // the leftmost pointer
            std::memcpy(dest, src, bytesToCopy);

            uint32_t splitKey = *reinterpret_cast<uint32_t*>(
                node + (splitPos + 1) * 8);

            // trim the left node
            *reinterpret_cast<uint32_t*>(node) = splitPos * 8 + 4;

            // Now, insert the key & value
            bool insertRight = pos > splitPos + 1;
            insertRaw(insertRight ? rightNode : node ,
                insertRight ? (pos - splitPos - !leafFlag) : pos, key, value);
                // If inserting in the rightNode, we need to shift
                // the position by 1 slot more if the node is an internal
                // node, to account for the fact that the middle key
                // is moved to the parent

            key = splitKey;
            value = rightRef;

            if (level == cursor.levels()) break;
            --level;
            ++level->pos;
        }


        // Create a new root level

        auto [rootRef, rootNode] = Derived::allocNode(tx);
        uint32_t* pInt = reinterpret_cast<uint32_t*>(rootNode);
        pInt[0] = 12;
        pInt[1] = *root;
        pInt[2] = static_cast<uint32_t>(key);
        pInt[3] = value;
        *root = rootRef;
    }

    /// @brief Deletes a key/value pair from a node, without
    /// attempting to simplify the tree.
    ///
    static void removeRaw(uint8_t* node, int pos)
    {
        uint8_t* p = Derived::ptrOfEntry(node, pos+1);
        uint8_t* end = node + Derived::nodeSize(node);
        if(p < end)
        {
            std::memmove(p - 8, p, end - p);
        }
        *reinterpret_cast<uint32_t*>(node) -= 8;
    }

    static void remove(Cursor& cursor)
    {
        Transaction* tx = cursor.transaction();
        auto minSize = Derived::minNodeSize(tx);
        Level* level = cursor.leaf();
        for (;;)
        {
            uint8_t* node = level->node;
            removeRaw(node, level->pos);
            auto nodeSize = Derived::nodeSize(node);
            if (nodeSize >= minSize) return;

            bool isLeaf = Derived::isLeaf(node);
            if (level == cursor.levels())
            {
                // We're at the root

                if (!isLeaf && nodeSize == 8)
                {
                    // If the root is an internal node and
                    // has one remaining pointer (the leftmost),
                    // delete this root and set the child as the new root
                    uint32_t child = *reinterpret_cast<uint32_t*>(node + 4);
                    Derived::freeNode(tx, cursor.root());
                    cursor.setRoot(child);
                }
                return;
            }


            uint8_t* leftNode = nullptr;
            uint8_t* rightNode = nullptr;
            size_t leftSize;
            size_t rightSize;
            --level;
            uint32_t* pParentSlot = reinterpret_cast<uint32_t*>(
                 level->node + level->pos * 8 + 4);
            if (reinterpret_cast<uint8_t*>(pParentSlot) > level->node + 4)
            {
                // child node has a left sibling
                leftNode = Derived::getNode(tx, *(pParentSlot-2));
                leftSize = Derived::nodeSize(leftNode);
                if (leftSize > minSize)
                {
                    // can borrow from left sibling
                    uint32_t borrowedKey = *reinterpret_cast<uint32_t*>(
                        leftNode + leftSize - 8);
                    uint32_t borrowedValue = *reinterpret_cast<uint32_t*>(
                        leftNode + leftSize - 4);
                    *reinterpret_cast<uint32_t*>(leftNode) -= 8;
                    if (isLeaf)
                    {
                        insertRaw(node, 1, borrowedKey, borrowedValue);
                    }
                    else
                    {
                        std::memmove(node+12, node+4, nodeSize-4);
                        *reinterpret_cast<uint32_t*>(node+4) = borrowedValue;
                        *reinterpret_cast<uint32_t*>(node+8) = *(pParentSlot-1);
                        *reinterpret_cast<uint32_t*>(node) += 8;
                    }
                    *(pParentSlot-1) = borrowedKey;
                    return;
                }
            }

            if (reinterpret_cast<uint8_t*>(pParentSlot) < level->node + Derived::nodeSize(level->node) - 4)
            {
                // child node has a right sibling

                rightNode = Derived::getNode(tx, *(pParentSlot+2));
                rightSize = Derived::nodeSize(rightNode);
                if (rightSize > minSize)
                {
                    // can borrow from right sibling
                    uint32_t borrowedKey = *reinterpret_cast<uint32_t*>(
                        rightNode + 8);
                    uint8_t* pDest = rightNode + (isLeaf ? 8 : 4);
                    uint32_t borrowedValue = *reinterpret_cast<uint32_t*>(pDest + (isLeaf ? 4 : 0));
                    memmove(pDest, pDest + 8, rightSize - (isLeaf ? 16 : 12));
                    *reinterpret_cast<uint32_t*>(rightNode) -= 8;

                    if (isLeaf)
                    {
                        *reinterpret_cast<uint32_t*>(node + nodeSize) = borrowedKey;
                        *(pParentSlot+1) = *reinterpret_cast<uint32_t*>(rightNode + 8);
                    }
                    else
                    {
                        *reinterpret_cast<uint32_t*>(node + nodeSize) = *(pParentSlot+1);
                        *(pParentSlot+1) = borrowedKey;
                    }
                    *reinterpret_cast<uint32_t*>(node + nodeSize + 4) = borrowedValue;
                    *reinterpret_cast<uint32_t*>(node) += 8;
                    return;
                }
            }

            if (leftNode)
            {
                rightNode = node;
                rightSize = nodeSize;
            }
            else
            {
                assert(rightNode);
                leftNode = node;
                leftSize = nodeSize;
                ++level->pos;
                pParentSlot += 2;        // This is a uint32_t pointer
            }

            if (isLeaf)
            {
                memcpy(leftNode+leftSize, rightNode+8, rightSize-8);
                *reinterpret_cast<uint32_t*>(leftNode) = leftSize + rightSize - 12;
            }
            else
            {
                memcpy(leftNode+leftSize+4, rightNode+4, rightSize-4);
                *reinterpret_cast<uint32_t*>(leftNode+leftSize) = *(pParentSlot-1);
                *reinterpret_cast<uint32_t*>(leftNode) = leftSize + rightSize - 4;
            }
            Derived::freeNode(tx, *pParentSlot);
        }
    }

    void init(Transaction* tx, Value* root)
    {
        auto [ref, node] = Derived::allocNode(tx);
        *reinterpret_cast<uint64_t*>(node) = 4;
        *root = ref;
    }

    /*
    Error verifyNode(Transaction* tx, uint8_t* node, Key minKey, Key maxKey)
    {

    }
    */
};

} // namespace clarisma
