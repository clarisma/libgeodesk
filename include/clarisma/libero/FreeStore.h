// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/io/File.h>
#include <clarisma/io/FileBuffer3.h>
#include <clarisma/io/MemoryMapping.h>
#include <clarisma/util/DateTime.h>

namespace clarisma {

class StoreException : public IOException
{
public:
	explicit StoreException(const char* message)
		: IOException(message) {}

	explicit StoreException(const std::string& message)
		: IOException(message) {}

	explicit StoreException(const std::string& fileName, const char* message)
		: IOException(fileName + ": " + message) {}

	explicit StoreException(const std::string& fileName, const std::string& message)
		: IOException(fileName + ": " + message) {}
};


class FreeStore
{
public:
	class Journal;
	class Transaction;

	virtual ~FreeStore() {}

	enum class OpenMode
	{
		WRITE = 1,
		CREATE = 2,
		EXCLUSIVE = 4,
		TRY_EXCLUSIVE = 8
	};

	void open(const char* fileName, OpenMode mode);
	// void open(const char* fileName, Transaction* tx);
	void close();

protected:
	struct BasicHeader
	{
		uint32_t magic;
		uint16_t versionLow;
		uint16_t versionHigh;
		uint64_t commitId;
		uint8_t pageSizeShift;
		uint8_t activeSnapshot;
		uint16_t reserved;
		uint32_t reserved2;
	};

	struct Header : BasicHeader
	{
		uint32_t totalPages;
		uint32_t freeRangeIndex;
		uint32_t freeRanges;
	};

	static constexpr int BLOCK_SIZE = 4096;
	static constexpr int HEADER_SIZE = 512;
	static constexpr uint64_t SEGMENT_LENGTH = 1024 * 1024 * 1024;	// 1 GB
	static constexpr int LOCK_OFS = HEADER_SIZE;
	static constexpr int CHECKSUMMED_HEADER_SIZE = HEADER_SIZE - sizeof(uint32_t) * 2;
	static constexpr uint32_t INVALID_FREE_RANGE_INDEX = 0xffff'ffff;

	struct HeaderBlock : Header
	{
		uint8_t reserved[CHECKSUMMED_HEADER_SIZE - sizeof(Header)];
		uint32_t checksum;
		uint32_t unused;
	};

	static_assert(sizeof(HeaderBlock) == HEADER_SIZE);

	enum class JournalStatus
	{
		NONE,
		MISSING,
		SKIPPED,
		RETRY,
		ROLLED_BACK
	};

	static constexpr uint64_t JOURNAL_END_MARKER_FLAG = 0x8000'0000'0000'0000ULL;

	// 0 = no journaling needed
	// 1 = applied journal, txid may be different
	// -1 = need to retry (other process applying journal)
	static int ensureIntegrity(
		const char* storeFileName, FileHandle storeHandle,
		HeaderBlock* header,
		const char* journalFileName, bool isWriter);
	static bool verifyJournal(std::span<const byte> journal);
	static void applyJournal(FileHandle writableStore,
		std::span<const byte> journal,
		HeaderBlock* header);
	static bool verifyHeader(const HeaderBlock* header);
	const char* journalFileName() const
	{
		return journalFileName_.c_str();
	}

	virtual void gatherUsedRanges(std::vector<uint64_t>& ranges) = 0;
	uint32_t pagesForBytes(uint32_t bytes) const
	{
		return (bytes + (1 << pageSizeShift_) - 1) >> pageSizeShift_;
	}

private:
	File file_;
	uint32_t pageSizeShift_ = 12;	// TODO: default 4KB page
	bool writeable_ = false;
	bool lockedExclusively_ = false;
	MemoryMapping mapping_;
	std::string journalFileName_;
};

CLARISMA_ENUM_FLAGS(FreeStore::OpenMode)

} // namespace clarisma
