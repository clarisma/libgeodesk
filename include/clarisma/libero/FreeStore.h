// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/io/File.h>
#include <clarisma/io/FileBuffer3.h>
#include <clarisma/util/Crc32C.h>
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
	class Transaction;

	virtual ~FreeStore() {}

	void open(const char* fileName);
	void open(const char* fileName, Transaction* tx);

protected:
	struct BasicHeader
	{
		uint32_t magic;
		uint16_t versionLow;
		uint16_t versionHigh;
		uint32_t checksum;
		uint8_t dirtyAllocFlag;
		uint8_t pageSizeShift;
		uint16_t reserved;
		uint64_t transactionId;
	};

	struct Header : BasicHeader
	{

		uint32_t totalPages;
		uint32_t freeRangeIndex;
	};

	static constexpr int BLOCK_SIZE = 4096;
	static constexpr uint64_t SEGMENT_LENGTH = 1024 * 1024 * 1024;	// 1 GB

	struct HeaderBlock : Header
	{
		uint8_t unused[4096 - sizeof(Header)];
	};

	static_assert(sizeof(HeaderBlock) == BLOCK_SIZE);

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
		HeaderBlock* header, bool isHeaderValid);
	static bool verifyHeader(HeaderBlock* header);
	static void sealHeader(HeaderBlock* header);
	std::string getJournalFileName() const
	{
		return fileName_ + ".journal";
	}

	virtual void gatherUsedRanges(std::vector<uint64_t>& ranges) = 0;
	uint32_t pagesForBytes(uint32_t bytes) const
	{
		return (bytes + (1 << pageSizeShift_) - 1) >> pageSizeShift_;
	}

	FileHandle file() { return file_; }
	uint32_t pageSizeShift() const { return pageSizeShift_; }

private:
	File file_;
	std::string fileName_;
	uint32_t pageSizeShift_ = 12;	// TODO: default 4KB page
};

} // namespace clarisma
