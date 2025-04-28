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
        return nodes_[page];
    }

    BTreeData::PageNum allocMetaPage()
    {
        pageCount_++;
        nodes_[pageCount_] = new uint8_t[4096];
        return pageCount_;
    }

private:
    HashMap<BTreeData::PageNum,uint8_t*> nodes_;
    uint32_t pageCount_ = 0;
};

TEST_CASE("BlobStoreTree")
{
    MockTransaction tx;
    BTree<MockTransaction> tree;

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
    BTree<MockTransaction> tree;

    // one-time set-up, e.g. in main()
    std::random_device rd;                         // nondet seed
    std::mt19937_64    rng{rd()};                  // 64-bit Mersenne Twister
    std::uniform_int_distribution<uint32_t> dist(1, 250'000); // inclusive bounds

    int targetCount = 1000000;
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


}

