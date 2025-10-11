// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <memory>
#include <random>
#include <string_view>
#include <catch2/catch_test_macros.hpp>
#include <clarisma/cli/Console.h>
#include <clarisma/util/log.h>

#include "clarisma/libero/FreeStore_Transaction.h"

using namespace clarisma;

class TestFreeStore : public FreeStore
{
public:
	class Transaction : public FreeStore::Transaction
	{
	public:
		using FreeStore::Transaction::Transaction;

		void createStore()
		{
			beginCreateStore();
			header().magic = 0x4321;
			endCreateStore();
		}
	};


protected:
	void gatherUsedRanges(std::vector<uint64_t>& ranges) override
	{
	}
};


TEST_CASE("FreeStore")
{
	const char *filename = R"(d:\geodesk\tests\freestore.bin)";

	Console console;
	TestFreeStore store;

	std::remove(filename);
	store.open(filename, FreeStore::OpenMode::WRITE | FreeStore::OpenMode::CREATE);
	TestFreeStore::Transaction t0(store);
	t0.begin();
	t0.createStore();
	t0.commit();
	t0.end();

	TestFreeStore::Transaction t1(store);
	t1.begin();
	uint32_t a = t1.allocPages(20);
	uint32_t b = t1.allocPages(1000);
	uint32_t c = t1.allocPages(200'000);
	uint32_t d = t1.allocPages(100'000);
	t1.dumpFreeRanges();
	t1.commit();
	t1.end();

	TestFreeStore::Transaction t2(store);
	t2.begin();
	t2.freePages(a, 20);
	t2.commit();
	t2.dumpFreeRanges();
	t2.end();

	TestFreeStore::Transaction t3(store);
	t3.begin();
	t3.freePages(b, 1000);
	t3.commit();
	t3.dumpFreeRanges();
	t3.end();

	store.close();
}


TEST_CASE("FeatureStore simulation")
{
	std::random_device rd;                         // nondet seed
	std::mt19937_64    rng{rd()};                  // 64-bit Mersenne Twister

	struct Blob
	{
		uint32_t firstPage;
		uint32_t pages;
	};

	std::vector<Blob> tiles;

	const char *filename = R"(d:\geodesk\tests\fstore-sim2.bin)";

	Console console;
	TestFreeStore store;
	std::remove(filename);

	store.open(filename, FreeStore::OpenMode::WRITE | FreeStore::OpenMode::CREATE);
	TestFreeStore::Transaction t0(store);
	t0.begin();
	t0.createStore();
	std::uniform_int_distribution<uint32_t> initialSizeRange(100, 1000);

	int tileCount = 60000;
	for (int i= 0; i<tileCount; i++)
	{
		int pages = initialSizeRange(rng);
		int firstPage = t0.allocPages(pages);
		tiles.emplace_back(firstPage, pages);
	}
	t0.commit();
	t0.end();

	int typicalEdits = 200;

	std::uniform_int_distribution<uint32_t> editCountRange(
		typicalEdits-100, typicalEdits + 100);

	std::vector<Blob> edited;

	TestFreeStore::Transaction tx(store);
	tx.begin();

	for (int i=0; i<1000; i++)
	{
		int editCount = editCountRange(rng);
		edited.reserve(editCount);
		for (int i2 = 0; i2<editCount; i2++)
		{
			std::uniform_int_distribution<size_t> tileRange(0, tiles.size()-1);
			size_t index = tileRange(rng);
			auto [firstPage, pages] = tiles[index];
			edited.emplace_back(firstPage, pages);
			tx.freePages(firstPage, pages);
			tiles.erase(tiles.begin() + index);
		}

		for (int i2 = 0; i2<editCount; i2++)
		{
			auto [firstPage, pages] = edited[i2];
			uint32_t minPages = pages < 51 ? 50 : (pages-1);
			uint32_t maxPages = pages > 1495 ? 1500 : (pages + 5);
			std::uniform_int_distribution<uint32_t> newSizeRange(
				minPages, maxPages);
			pages = newSizeRange(rng);
			firstPage = tx.allocPages(pages);
			// tx.checkFreeTrees();
			tiles.emplace_back(firstPage, pages);
		}
		edited.clear();

		LOGS << "Committing #" << i << "\n";
		tx.commit();
		LOGS << "Committed #" << i << "\n";
		printf("Committed #%d\n", i);
		fflush(stdout);
		// tx.dumpFreeRanges();
	}

	tx.dumpFreeRanges();
	size_t tilePages = 0;
	for (const auto& e : tiles)
	{
		tilePages += e.pages;
	}
	LOGS << tiles.size() << " tiles with " << tilePages << " pages\n";

	tx.end();
	store.close();
}

