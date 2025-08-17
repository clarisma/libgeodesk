// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/io/File.h>
#include <clarisma/io/FileBuffer3.h>
#include <clarisma/util/DateTime.h>

#include "clarisma/data/BTreeSet.h"
#include "clarisma/data/HashSet.h"
#include "clarisma/util/Crc32.h"

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
	class Transaction;

public:
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

	class Transaction
	{
	public:
		Transaction(FreeStore& store);

		void begin();

		/// @brief Adds a 4 KB block to the Journal, so it can
		/// be safely overwritten once save() has been called.
		/// The block must be aligned at a 4 KB boundary.
		/// If the block has been added to the Journal since
		/// the last call to commit, this method does nothing.
		///
		/// @ofs     the offset of the block (must be 4-KB aligned)
		/// @content the block's current content (must be 4 KB in length)
		///
		void stageBlock(uint64_t ofs, const void* content);
		void save();
		void commit();
		void end();

	private:
		void buildFreeRangeIndex();

		FreeStore& store_;
		File journalFile_;
		FileBuffer3 journalBuffer_;
		HashSet<uint64_t> stagedBlocks_;
		BTreeSet<uint64_t> freeBySize_;
		BTreeSet<uint64_t> freeByStart_;
		uint32_t totalPages_;
		uint32_t freeRangeCount_;
		Crc32 journalChecksum_;
		bool allocationChanged_ = false;
		HeaderBlock header_;
	};


private:
	File file_;
	std::string fileName_;
};

} // namespace clarisma
