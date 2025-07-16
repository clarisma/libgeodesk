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
    HashMap<uint32_t,uint8_t*> nodes_;
    uint32_t pageCount_ = 0;
};

class TestBTree : public BTree<TestBTree,MockTransaction,8>
{
public:
    static size_t maxNodeSize(MockTransaction* tx)
    {
        return 64;
    }

    static uint8_t* getNode(MockTransaction* tx, Value ref)    // CRTP override
    {
        assert(tx->nodes_.contains(ref));
        return tx->nodes_[ref];
    }

    static std::pair<Value,uint8_t*> allocNode(MockTransaction* tx)           // CRTP override
    {
        tx->pageCount_++;
        uint8_t* node = new uint8_t[maxNodeSize(tx)];
        tx->nodes_[tx->pageCount_] = node;
        return { tx->pageCount_, node };
    }

    void init(MockTransaction* tx)
    {
        BTree::init(tx, &root_);
    }

    void insert(MockTransaction* tx, Key key, Value value)
    {
        BTree::insert(tx, &root_, key, value);
    }

    Value root_;
};



TEST_CASE("BlobStoreTree")
{
    MockTransaction tx;
    TestBTree tree;
    tree.init(&tx);

    tree.insert(&tx, 10, 1000);
    tree.insert(&tx, 20, 2000);
    tree.insert(&tx, 30, 3000);
    tree.insert(&tx, 15, 1500);

    /*
    auto iter = tree.iter(&tx);
    while (iter.hasMore())
    {
        auto& e = iter.next();
        std::cout << e.key << " = " << e.value << std::endl;
    }
    */
}


/*
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

*/