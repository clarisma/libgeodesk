// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <memory>
#include <random>
#include <string_view>
#include <catch2/catch_test_macros.hpp>
#include <clarisma/cli/Console.h>
#include <clarisma/util/log.h>

#include "clarisma/store/BlobStore2.h"

using namespace clarisma;

class TestBlobStore : public BlobStore2
{
public:
	class Transaction : public BlobStore2::Transaction
	{
	public:
		using BlobStore2::Transaction::Transaction;

		void createStore()
		{
			beginCreateStore();
			endCreateStore();
		}
	};
};

TEST_CASE("BlobStore")
{
	const char *filename = R"(c:\geodesk\tests\blobstore.bin)";

	Console console;
	TestBlobStore store;

	std::remove(filename);
	store.open(filename, File::OpenMode::READ | File::OpenMode::WRITE |
		File::OpenMode::CREATE);
	TestBlobStore::Transaction t0(&store);
	t0.begin();
	t0.createStore();
	t0.commit();
	t0.end();

	TestBlobStore::Transaction t1(&store);
	t1.begin();
	uint32_t a = t1.allocPages(20);
	uint32_t b = t1.allocPages(1000);
	uint32_t c = t1.allocPages(200'000);
	uint32_t d = t1.allocPages(100'000);
	t1.dumpFreePages();
	t1.commit();
	t1.end();

	TestBlobStore::Transaction t2(&store);
	t2.begin();
	t2.freePages(a, 20);
	t2.commit();
	t2.dumpFreePages();
	t2.end();

	TestBlobStore::Transaction t3(&store);
	t3.begin();
	t3.freePages(b, 1000);
	t3.commit();
	t3.dumpFreePages();
	t3.end();

	store.close();
}


TEST_CASE("BlobStore Random Alloc & Free")
{
	std::random_device rd;                         // nondet seed
	std::mt19937_64    rng{rd()};                  // 64-bit Mersenne Twister

	struct Blob
	{
		uint32_t firstPage;
		uint32_t pages;
	};

	std::vector<Blob> blobs;

	const char *filename = R"(c:\geodesk\tests\blobstore-alloc.bin)";

	Console console;
	TestBlobStore store;
	std::remove(filename);

	store.open(filename, File::OpenMode::READ | File::OpenMode::WRITE |
		File::OpenMode::CREATE);
	TestBlobStore::Transaction t0(&store);
	t0.begin();
	t0.createStore();
	t0.commit();
	t0.end();

	int maxInserts = 10;
	int maxBlobSize = 10000; // 1 << 18;

	std::uniform_int_distribution<uint32_t> insertCountRange(1, maxInserts);
	std::uniform_int_distribution<uint32_t> insertSizeRange(1, maxBlobSize);

	TestBlobStore::Transaction tx(&store);
	tx.begin();

	for (int i=0; i<10000; i++)
	{
		int insertCount = insertCountRange(rng);
		for (int i2 = 0; i2<insertCount; i2++)
		{
			int pages = insertSizeRange(rng);
			int firstPage = tx.allocPages(pages);
			tx.checkFreeTrees();
			blobs.emplace_back(firstPage, pages);
		}

		int deleteCount = insertCountRange(rng) * 7 / 8;
		for (int i2 = 0; i2<deleteCount; i2++)
		{
			if (blobs.empty()) break;
			std::uniform_int_distribution<size_t> entryRange(0, blobs.size()-1);
			size_t index = entryRange(rng);
			auto [firstPage, pages] = blobs[index];
			tx.freePages(firstPage, pages);
			blobs.erase(blobs.begin() + index);
		}
		LOGS << "Committing #" << i << "\n";
		tx.commit();
		LOGS << "Committed #" << i << "\n";
		tx.checkFreeTrees();
	}

	tx.dumpFreePages();
	LOGS << blobs.size() << " blobs\n";

	tx.end();
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

	const char *filename = R"(c:\geodesk\tests\fstore-sim2.bin)";

	Console console;
	TestBlobStore store;
	std::remove(filename);

	store.open(filename, File::OpenMode::READ | File::OpenMode::WRITE |
		File::OpenMode::CREATE);
	TestBlobStore::Transaction t0(&store);
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

	TestBlobStore::Transaction tx(&store);
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
		// tx.checkFreeTrees();
	}

	tx.dumpFreePages();
	size_t tilePages = 0;
	for (const auto& e : tiles)
	{
		tilePages += e.pages;
	}
	LOGS << tiles.size() << " tiles with " << tilePages << " pages\n";

	tx.end();
	store.close();
}


TEST_CASE("BlobStore Random Alloc & Free")
{
	std::random_device rd;                         // nondet seed
	std::mt19937_64    rng{rd()};                  // 64-bit Mersenne Twister

	struct Blob
	{
		uint32_t firstPage;
		uint32_t pages;
	};

	std::vector<Blob> blobs;

	const char *filename = R"(c:\geodesk\tests\blobstore-alloc.bin)";

	Console console;
	TestBlobStore store;
	std::remove(filename);

	store.open(filename, File::OpenMode::READ | File::OpenMode::WRITE |
		File::OpenMode::CREATE);
	TestBlobStore::Transaction t0(&store);
	t0.begin();
	t0.createStore();
	t0.commit();
	t0.end();

	int maxInserts = 1000;
	int maxBlobSize = 10000; // 1 << 18;

	std::uniform_int_distribution<uint32_t> insertCountRange(1, maxInserts);
	std::uniform_int_distribution<uint32_t> insertSizeRange(1, maxBlobSize);

	TestBlobStore::Transaction tx(&store);
	tx.begin();

	for (int i=0; i<100; i++)
	{
		int insertCount = insertCountRange(rng);
		for (int i2 = 0; i2<insertCount; i2++)
		{
			int pages = insertSizeRange(rng);
			int firstPage = tx.allocPages(pages);
			// tx.checkFreeTrees();
			blobs.emplace_back(firstPage, pages);
		}

		int deleteCount = insertCountRange(rng) * 7 / 8;
		for (int i2 = 0; i2<deleteCount; i2++)
		{
			if (blobs.empty()) break;
			std::uniform_int_distribution<size_t> entryRange(0, blobs.size()-1);
			size_t index = entryRange(rng);
			auto [firstPage, pages] = blobs[index];
			tx.freePages(firstPage, pages);
			blobs.erase(blobs.begin() + index);
		}
		LOGS << "Committing #" << i << "\n";
		tx.commit();
		LOGS << "Committed #" << i << "\n";
		printf("Committed #%d\n", i);
		fflush(stdout);
		// tx.checkFreeTrees();
	}

	tx.dumpFreePages();
	LOGS << blobs.size() << " blobs\n";

	tx.end();
	store.close();
}


