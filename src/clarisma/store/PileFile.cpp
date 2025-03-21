// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/store/PileFile.h>

#include <cassert>
#include <clarisma/util/Bits.h>
#include <clarisma/util/Bytes.h>

namespace clarisma {

// TODO: Implement open()!

PileFile::PileFile()
{

}

void PileFile::openExisting(const char* fileName)
{
	file_.open(fileName, File::OpenMode::READ | File::OpenMode::WRITE);
	// Open mode is implicitly sparse
	Metadata* meta = metadata();
	pageSizeShift_ = meta->pageSizeShift;
	pageSize_ = pageSizeShift_;
}


void PileFile::create(const char* fileName, uint32_t pileCount, 
	uint32_t pageSize, uint32_t preallocatePages)
{
	assert(pileCount > 0 && pileCount < MAX_PILE_COUNT);
	assert(pageSize >= 4096 && pageSize <= ExpandableMappedFile::SEGMENT_LENGTH 
		&& Bytes::isPowerOf2(pageSize));


	uint32_t entriesPerPage = pageSize / sizeof(IndexEntry);
	uint32_t pageCount = (pileCount + entriesPerPage) / entriesPerPage;
	// No need to use -1, because count includes the metadata (which 
	// has the same size as an index entry)

	int openMode = File::OpenMode::READ | File::OpenMode::WRITE;
	if (preallocatePages)
	{
		File file;
		file.open(fileName, File::OpenMode::READ | File::OpenMode::WRITE | 
			File::OpenMode::CREATE | File::OpenMode::REPLACE_EXISTING);
		
		size_t size = static_cast<size_t>(pageSize) * (pageCount + preallocatePages);
		file.setSize(size);
        #ifdef _WIN32
		file.zeroFill(0, size);
		file.makeSparse();
        #else
        file.allocate(0, size);
        #endif
		file.close();
	}
	else
	{
		openMode |= File::OpenMode::CREATE | File::OpenMode::REPLACE_EXISTING;
	}
	file_.open(fileName, openMode);
		// Open mode is implicitly sparse

	Metadata* meta = metadata();
	pageSize_ = pageSize;
	pageSizeShift_ = Bits::countTrailingZerosInNonZero(pageSize);
	meta->magic = MAGIC;
	meta->pageCount = pageCount;
	meta->pileCount = pileCount;
	meta->pageSizeShift = pageSizeShift_;
}


void PileFile::preallocate(int pile, int pages)
{
	Metadata* meta = metadata();
	IndexEntry* indexEntry = &(meta->index[pile - 1]);
	assert(indexEntry->firstPage == 0);
	assert(indexEntry->lastPage == 0);
	assert(indexEntry->totalPayloadSize == 0);
	assert((static_cast<uint64_t>(pages) << pageSizeShift_) < (1ULL << 32));
	// Don't go through regular allocation, since that would cause a write
	// in the middle of the file; more efficient to defer it and let append()
	// handle the first-chunk initialization
	indexEntry->firstPage = meta->pageCount;
	// We leave indexEntry->lastPage = 0 to indicate that the first chunk is 
	// allocated, but uninitialized; instead, we set the totalPayloadSize
	// to the allocated payload size -- append() will then reset this to 0
	indexEntry->totalPayloadSize = static_cast<uint32_t>(pages << pageSizeShift_) -
		CHUNK_HEADER_SIZE;
	meta->pageCount += pages;

	// Console::msg("Preallocated pile %d: %llu bytes", pile, indexEntry->totalPayloadSize);
}


void PileFile::append(int pile, const uint8_t* data, uint32_t len)
{
	assert (pile > 0 && pile <= metadata()->pileCount);
	IndexEntry* indexEntry = &metadata()->index[pile - 1];
	uint32_t lastPage = indexEntry->lastPage;
	Chunk* chunk;
	uint32_t chunkSize;

	if (lastPage == 0)
	{
		// First write to a chunk
		if (indexEntry->firstPage == 0)
		{
			// Pile does not yet exist
			assert(indexEntry->totalPayloadSize == 0);
			ChunkAllocation alloc = allocChunk(len);
			lastPage = alloc.firstPage;
			indexEntry->firstPage = lastPage;
			indexEntry->lastPage = lastPage;
			chunk = alloc.chunk;
		}
		else
		{
			// Pre-allocated chunk
			chunk = getChunk(indexEntry->firstPage);
			assert(chunk->payloadSize == 0 && chunk->remainingSize == 0);
			assert(indexEntry->totalPayloadSize < (1ULL << 32));
			chunkSize = static_cast<uint32_t>(indexEntry->totalPayloadSize);
			chunk->payloadSize = chunkSize;
			chunk->remainingSize = chunkSize;
			indexEntry->totalPayloadSize = 0;
			indexEntry->lastPage = indexEntry->firstPage;
		}
	}
	else
	{
		chunk = getChunk(lastPage);
	}

	chunkSize = chunk->payloadSize;
	uint32_t remainingSize = chunk->remainingSize;
	assert(chunkSize >= remainingSize);

	uint32_t n = std::min(len, remainingSize);
	memcpy(&chunk->data[chunkSize - remainingSize], data, n);
		// TODO: technically could be UB if n==0, since dest now points to potentially
		// invalid memory
	chunk->remainingSize -= n;
	uint32_t leftToWrite = len - n;
	if (leftToWrite > 0)
	{
		ChunkAllocation alloc = allocChunk(leftToWrite);
		chunk->nextPage = alloc.firstPage;
		assert(chunk->payloadSize > 0);
		assert(chunk->nextPage != 0);
		indexEntry->lastPage = alloc.firstPage;
		assert(alloc.chunk != chunk);
		chunk = alloc.chunk;
		memcpy(&chunk->data, &data[n], leftToWrite);
		chunk->remainingSize -= leftToWrite;
		assert(chunk->payloadSize > 0);
	}
	indexEntry->totalPayloadSize += len;
}


void PileFile::load(int pile, ReusableBlock& block)
{
	assert (pile > 0 && pile <= metadata()->pileCount);
	IndexEntry* indexEntry = &metadata()->index[pile - 1];
	uint32_t page = indexEntry->firstPage;
	uint32_t lastPage = indexEntry->lastPage;
	if (lastPage == 0)
	{
		// Console::msg("Reading pile %d (empty)...", pile);
		block.reset(0);
		return;
	}
	size_t dataSize = indexEntry->totalPayloadSize;
	// Console::msg("Reading pile %d (%llu bytes)...", pile, dataSize);
	block.reset(dataSize);
	uint8_t* pStart = block.data();
	uint8_t* p = pStart;
	size_t leftToRead = dataSize;
	for(;;)
	{
		const Chunk* chunk = getChunk(page);
		if (page == lastPage)
		{
			size_t finalChunkSize = chunk->payloadSize - chunk->remainingSize;
			//Console::msg("  [%d] Final chunk (page %d): %lld bytes", pile, page, finalChunkSize);
			assert(leftToRead == finalChunkSize);
			memcpy(p, &chunk->data, finalChunkSize);
			break;
		}
		//Console::msg("  [%d] Chunk at page %d: %lld bytes", pile, page, chunk->payloadSize);
		size_t chunkSize = chunk->payloadSize;
		assert(leftToRead > chunkSize);
		memcpy(p, chunk->data, chunkSize);
		p += chunkSize;
		leftToRead -= chunkSize;
		page = chunk->nextPage;
	}
}


PileFile::ChunkAllocation PileFile::allocChunk(uint32_t minPayload)
{
	assert(minPayload > 0);
	uint32_t firstPage = metadata()->pageCount;
	uint32_t pages =
		static_cast<uint32_t>(
			(minPayload + pageSize_ - CHUNK_HEADER_SIZE - 1)
			/ (pageSize_ - CHUNK_HEADER_SIZE));
	metadata()->pageCount = firstPage + pages;
	// Console::msg("Allocated %d pages", pages);
	Chunk* chunk = getChunk(firstPage);
	chunk->payloadSize = static_cast<uint32_t>(pages << pageSizeShift_) - CHUNK_HEADER_SIZE;
	chunk->remainingSize = chunk->payloadSize;
	return ChunkAllocation{ chunk, firstPage };
}

PileFile::Chunk* PileFile::getChunk(uint32_t page)
{
	return reinterpret_cast<Chunk*>(file_.translate(
		static_cast<uint64_t>(page) << pageSizeShift_));
}
} // namespace clarisma
