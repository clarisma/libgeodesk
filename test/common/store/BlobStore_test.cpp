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


/*

std::unique_ptr<uint8_t[]> createJunk(size_t size)
{
	// Define the pattern "ABCDEFG..."
	const char* pattern = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	size_t patternLength = strlen(pattern);

	// Allocate the memory block
	std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);

	// Fill the buffer with the repeating pattern
	for (size_t i = 0; i < size; ++i)
	{
		buffer[i] = pattern[i % patternLength];
	}

	return buffer;
}


TEST_CASE("BlobStore")
{
	const char *filename = R"(c:\geodesk\tests\blobstore.bin)";
	BlobStore::CreateTransaction<BlobStore> t0;
	t0.begin(filename);
	t0.commit();
	t0.end();

	BlobStore store;
	store.open(filename, File::OpenMode::READ | File::OpenMode::WRITE);
	BlobStore::Transaction t1(&store);
	std::string_view dataA("Test data");
	std::string_view dataB("More test data");
	std::string_view dataC("Even more test data");
	BlobStore::PageNum a = t1.addBlob(ByteSpan(reinterpret_cast<const uint8_t*>(dataA.data()), dataA.size()));
	BlobStore::PageNum b = t1.addBlob(ByteSpan(reinterpret_cast<const uint8_t*>(dataB.data()), dataB.size()));
	t1.commit();
	t1.end();

	BlobStore::Transaction t2(&store);
    t2.begin();
	t2.free(a);
	t2.commit();
	t2.end();

	BlobStore::Transaction t3(&store);
	t3.begin();
	BlobStore::PageNum c = t3.addBlob(ByteSpan(reinterpret_cast<const uint8_t*>(dataC.data()), dataC.size()));
    t3.commit();
	t3.end();

	store.close();
}


TEST_CASE("BlobStore Hole-Punching")
{
	const char *filename = R"(c:\geodesk\tests\blobstore-holes.bin)";
	BlobStore::CreateTransaction<BlobStore> t0;
	t0.begin(filename);
	t0.commit();
	t0.end();

	BlobStore store;
	store.open(filename, File::READ | File::WRITE);
	BlobStore::Transaction t1(&store);
    t1.begin();
	size_t chunkSize = 16 * 1024 * 1024;
	std::unique_ptr<uint8_t[]> junk = createJunk(chunkSize);
	BlobStore::PageNum a = t1.addBlob(ByteSpan(junk.get(), chunkSize));
	BlobStore::PageNum b = t1.alloc(3 * 1024); 
	t1.commit();
	t1.end();

	BlobStore::Transaction t2(&store);
    t2.begin();
	t2.free(a);
	t2.commit();
	t2.end();

	store.close();
}

*/