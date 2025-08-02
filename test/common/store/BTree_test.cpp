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

class TestBTree : public BTree<TestBTree,MockTransaction,uint32_t,uint32_t,8>
{
public:
    static size_t maxNodeSize(MockTransaction* tx)
    {
        // return 64;
        return 4096;
    }

    static uint8_t* getNode(MockTransaction* tx, uint32_t ref)    // CRTP override
    {
        assert(tx->nodes_.contains(ref));
        return tx->nodes_[ref];
    }

    static std::pair<NodeRef,uint8_t*> allocNode(MockTransaction* tx)           // CRTP override
    {
        tx->pageCount_++;
        uint8_t* node = new uint8_t[maxNodeSize(tx)];
        tx->nodes_[tx->pageCount_] = node;
        return { tx->pageCount_, node };
    }

    static void freeNode(MockTransaction* tx, NodeRef ref)           // CRTP override
    {
        assert(tx->nodes_.contains(ref));
        delete[] tx->nodes_[ref];
    }

    void init(MockTransaction* tx)
    {
        BTree::init(tx, &root_);
    }

    Iterator iter(MockTransaction* tx)
    {
        return Iterator(tx, &root_);
    }

    void insert(MockTransaction* tx, uint32_t key, uint32_t value)
    {
        Cursor cursor(tx, &root_);
        cursor.insert(key, value);
    }

    void removeFirst(MockTransaction* tx)
    {
        Cursor cursor(tx, &root_);
        cursor.moveToFirst();
        remove(cursor);
    }

    void removeLast(MockTransaction* tx)
    {
        Cursor cursor(tx, &root_);
        cursor.moveToLast();
        remove(cursor);
    }

    Entry takeLowerBound(MockTransaction* tx, uint32_t x)
    {
        return BTree::takeLowerBound(tx, &root_, x);
    }

    NodeRef root_;
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


TEST_CASE("Random BlobStoreTree")
{
    MockTransaction tx;
    TestBTree tree;
    tree.init(&tx);

    // one-time set-up, e.g. in main()
    std::random_device rd;                         // nondet seed
    std::mt19937_64    rng{rd()};                  // 64-bit Mersenne Twister
    std::uniform_int_distribution<uint32_t> dist(1, 250'000); // inclusive bounds

    int targetCount = 1000000;
    uint32_t hash = 0;

    for (int i=0; i<targetCount; i++)
    {
        uint32_t k = dist(rng);
        tree.insert(&tx, k, k * 100);
        hash ^= k;
    }

    int actualCount = 0;
    uint32_t actualHash = 0;
    TestBTree::Iterator it = tree.iter(&tx);
    while (it.hasNext())
    {
        auto [k,v] = it.next();
        ++actualCount;
        actualHash ^= k;
        // std::cout << k << " = " << v << std::endl;
    }

    REQUIRE(actualCount == targetCount);
    REQUIRE(actualHash == hash);

    int removeCount = targetCount / 2;
    for (int i=0; i<removeCount; i++)
    {
        uint32_t k = dist(rng);
        auto[ek,ev] = tree.takeLowerBound(&tx, k);
        if (ek)
        {
            REQUIRE(ek >= k);
            REQUIRE(ev == ek * 100);
            hash ^= ek;
            --targetCount;
        }
    }

    actualCount = 0;
    actualHash = 0;
    it = tree.iter(&tx);
    while (it.hasNext())
    {
        auto [k,v] = it.next();
        ++actualCount;
        actualHash ^= k;
    }

    REQUIRE(actualCount == targetCount);
    REQUIRE(actualHash == hash);


    /*
    for (int i=0; i<actualCount; i++)
    {
        tree.removeLast(&tx);
    }

    it = tree.iter(&tx);
    REQUIRE(!it.hasNext());
    */

}

