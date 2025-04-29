// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only


#include <iostream>
#include <memory>
#include <random>
#include <string_view>
#include <catch2/catch_test_macros.hpp>
#include <clarisma/store/BTree.h>
#include <clarisma/data/HashMap.h>
#include <clarisma/data/HashSet.h>


using namespace clarisma;

class MockTransaction
{
public:
    uint8_t* getBlobBlock(BTreeData::PageNum page)
    {
        assert(nodes_.contains(page));
        return nodes_[page];
    }

    BTreeData::PageNum allocMetaPage()
    {
        pageCount_++;
        nodes_[pageCount_] = new uint8_t[4096];
        return pageCount_;
    }

    void freeMetaPage(BTreeData::PageNum page)
    {
        assert(nodes_.contains(page));
        delete nodes_[page];
        nodes_.erase(page);
    }

private:
    HashMap<BTreeData::PageNum,uint8_t*> nodes_;
    uint32_t pageCount_ = 0;
};

using Tree = BTree<MockTransaction,7,16>;

/// Verify B-tree invariants below @p node.  Returns `true` so it can be
/// chained in REQUIRE() expressions.
///
/// Checks performed
///   * key count  ∈ [MIN_ENTRIES .. MAX_ENTRIES]  (root exempt from min)
///   * keys inside the node are non-decreasing
///   * all leaves are at the same depth
///   * separator keys correctly partition the child sub-ranges
///
void validateNode(MockTransaction& tx,
                  typename BTree<MockTransaction,7,16>::Node* node,
                  uint32_t depth,
                  uint32_t height,
                  bool isRoot,
                  uint32_t* outMinKey,
                  uint32_t* outMaxKey)
{
    using Tree = BTree<MockTransaction,7,16>;

    uint32_t count = node->count();
    if (!isRoot) REQUIRE(count >= Tree::MIN_ENTRIES);
    REQUIRE(count <= Tree::MAX_ENTRIES);

    // ---- 1.  keys are sorted inside the node --------------------------
    for (uint32_t i = 1; i < count; ++i)
    {
        REQUIRE(node->entries[i].key <= node->entries[i + 1].key);
    }

    if (node->isLeaf())
    {
        REQUIRE(depth + 1 == height);

        *outMinKey = node->entries[1].key;
        *outMaxKey = node->entries[count].key;
        return;
    }

    // ---- 2.  recurse into children and check separator keys -----------
    uint32_t childMin, childMax;

    // first child
    auto* child =
        reinterpret_cast<typename Tree::Node*>(
            tx.getBlobBlock(node->entries[0].child));
    validateNode(tx, child, depth + 1, height, false,
                 &childMin, &childMax);
    *outMinKey = childMin;

    for (uint32_t i = 1; i <= count; ++i)
    {
        uint32_t sepKey = node->entries[i].key;

        child =
            reinterpret_cast<typename Tree::Node*>(
                tx.getBlobBlock(node->entries[i].child));
        validateNode(tx, child, depth + 1, height, false,
                     &childMin, &childMax);

        // separator key must be ≥ max key of left child
        REQUIRE(childMax >= sepKey);
        // and < min key of *right* child (strictly, if you disallow dups)
        REQUIRE(sepKey <= childMin);

        *outMaxKey = childMax;
    }
}

void validateTree(MockTransaction& tx, const Tree tree)
{
    if (tree.data().height == 0)
        return;

    uint32_t minKey, maxKey;
    auto* root =
        reinterpret_cast<typename Tree::Node*>(
            tx.getBlobBlock(tree.data().root));

    validateNode(tx, root, 0, tree.data().height, true, &minKey, &maxKey);
}



TEST_CASE("BlobStoreTree")
{
    MockTransaction tx;
    BTree<MockTransaction, 7, 16> tree;

    tree.insert(&tx, 10, 1000);
    tree.insert(&tx, 20, 2000);
    tree.insert(&tx, 30, 3000);
    tree.insert(&tx, 15, 1500);

    auto iter = tree.iter(&tx);
    while (iter.hasMore())
    {
        auto& e = iter.next();
        std::cout << e.key << " = " << e.value << std::endl;
    }
}


TEST_CASE("Random BlobStoreTree")
{
    MockTransaction tx;
    BTree<MockTransaction, 7, 16> tree;

    // one-time set-up, e.g. in main()
    std::random_device rd;                         // nondet seed
    std::mt19937_64    rng{rd()};                  // 64-bit Mersenne Twister
    std::uniform_int_distribution<uint32_t> dist(1, 250'000); // inclusive bounds

    int targetCount = 10000;
    HashSet<uint32_t> keys;
    std::vector<BTreeData::Entry> items;

    for (int i=0; i<targetCount; i++)
    {
        uint32_t k = dist(rng);
        tree.insert(&tx, k, k * 100);
        keys.insert(k);
        /*
        if ((i + 1) % 1000 == 0)
        {
            std::cout << "Inserted " << (i+1);
        }
        */
        items.push_back(BTreeData::Entry{k, k * 100});
        REQUIRE(tree.count(&tx) == i + 1);
    }
    std::sort(items.begin(), items.end());

    HashSet<uint32_t> actualKeys;
    std::vector<BTreeData::Entry> actualItems;
    int count = 0;
    auto iter = tree.iter(&tx);
    while (iter.hasMore())
    {
        auto& e = iter.next();
        actualKeys.insert(e.key);
        actualItems.push_back(e);
        // std::cout << e.key << " = " << e.value << std::endl;
        count++;
    }
    std::sort(actualItems.begin(), actualItems.end());

    REQUIRE(items.size() == targetCount);
    REQUIRE(tree.count(&tx) == targetCount);
    REQUIRE(actualKeys.size() == keys.size());

    REQUIRE(count == targetCount);
    REQUIRE(actualKeys == keys);
    REQUIRE(actualItems == items);

    // Validate structure before we start deleting
    validateTree(tx, tree);

    // --- random deletes ------------------------------------------------
    std::ranges::shuffle(items, rng);
    int eraseCount = 0;

    for (auto const& e : items)
    {
        auto removed = tree.takeExact(&tx, e.key, e.value);
        REQUIRE(removed.key == e.key);
        REQUIRE(removed.value == e.value);
        eraseCount++;
        if (eraseCount % 17 == 0)          // arbitrary stride
        {
            validateTree(tx, tree);        // invariants still fine?
        }
    }

    REQUIRE(tree.count(&tx) == 0);
    REQUIRE(tree.data().height == 0);

    // tree should be empty; allocator should have reclaimed all but root
    // REQUIRE(tx.pageCount() == 0);          // add accessor in MockTransaction
}

