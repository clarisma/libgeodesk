// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only


#include <iostream>
#include <memory>
#include <random>
#include <string_view>
#include <catch2/catch_test_macros.hpp>
#include <clarisma/store/KeyTree.h>
#include <clarisma/data/HashMap.h>
#include <clarisma/data/HashSet.h>


using namespace clarisma;


class TestKeyTree : public KeyTree<TestKeyTree,8>
{
public:
    using KeyTree::KeyTree;

    static size_t maxNodeSize()
    {
        return 128;
        // return 4096;
    }

    static uint8_t* allocNode()           // CRTP override
    {
        return new uint8_t[maxNodeSize()];
    }

    static void freeNode(uint8_t* node)           // CRTP override
    {
        delete[] node;
    }

    /*
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
    */
};



TEST_CASE("KeyTree")
{
    TestKeyTree tree;

    tree.insert(10);
    tree.insert(20);
    tree.insert(30);
    tree.insert(15);

    auto iter = tree.iter();
    while (iter.hasNext())
    {
        auto e = iter.next();
        std::cout << e << std::endl;
    }
}


TEST_CASE("Random KeyTree")
{
    TestKeyTree tree;

    // one-time set-up, e.g. in main()
    std::random_device rd;                         // nondet seed
    std::mt19937_64    rng{rd()};                  // 64-bit Mersenne Twister
    std::uniform_int_distribution<uint64_t> dist(1, 2'000'000'000); // inclusive bounds

    int targetAttemptCount = 1000000;
    int targetCount = 0;
    uint32_t hash = 0;

    for (int i=0; i<targetAttemptCount; i++)
    {
        uint32_t k = dist(rng);
        // std::cout << "Inserting # << " << i << ": " << k << std::endl;
        bool inserted = tree.insert(k);
        targetCount += inserted;
        // tree.check();
        hash ^= inserted ? k : 0;
    }

    int actualCount = 0;
    uint32_t actualHash = 0;
    TestKeyTree::Iterator it = tree.iter();
    while (it.hasNext())
    {
        auto k = it.next();
        ++actualCount;
        actualHash ^= k;
        // std::cout << k << " = " << v << std::endl;
    }

    tree.check();
    REQUIRE(actualCount == targetCount);
    REQUIRE(actualHash == hash);

    int removeCount = targetCount / 2;
    for (int i=0; i<removeCount; i++)
    {
        uint32_t k = dist(rng);
        auto ek = tree.takeLowerBound(k);
        if (ek)
        {
            REQUIRE(ek >= k);
            hash ^= ek;
            --targetCount;
        }
    }

    actualCount = 0;
    actualHash = 0;
    it = tree.iter();
    while (it.hasNext())
    {
        auto k = it.next();
        ++actualCount;
        actualHash ^= k;
    }

    tree.check();
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

