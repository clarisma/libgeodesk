// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <unordered_map>
#include <vector>
#include <clarisma/io/FileLock.h>
#include <clarisma/io/ExpandableMappedFile.h>

namespace clarisma {

using std::byte;

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


class Store : protected ExpandableMappedFile
{
public:
	enum OpenMode
	{
		WRITE = 1 << 1,			// TODO: expected to match File::OpenMode
		CREATE = 1 << 2,		// TODO: expected to match File::OpenMode
								// TODO: Create always implies WRITE
		EXCLUSIVE = 1 << 7
	};

	Store();
	virtual ~Store() = default;

	void open(const char* filename, int /* OpenMode */ mode);
	void close();

	const std::string& fileName() const { return fileName_; }

protected:
	virtual void createStore() = 0;
	virtual void verifyHeader() const = 0;
	virtual void initialize() = 0;
	virtual uint64_t getLocalCreationTimestamp() const = 0;
	virtual uint64_t getTrueSize() const = 0;
	// void* mapSegment(uint32_t segNumber, uint32_t segCount);

	inline byte* data(uint64_t ofs)
	{
		return translate(ofs);
	}

	void error(const char* msg) const;

	enum LockLevel
	{
		LOCK_NONE = 0,
		LOCK_READ = 1,
		LOCK_APPEND = 2,
		LOCK_EXCLUSIVE = 3
	};

	class TransactionBlock
	{
	public:
		TransactionBlock(byte* original) :
			original_(original)
		{
			memcpy(current_, original, SIZE);
		}
		byte* original() { return original_; }
		byte* current() { return current_; }

		static const int SIZE = 4096;

	private:
		byte* original_;
		byte current_[SIZE];
	};

	class Transaction
	{
	public:
		Transaction(Store* store);
		~Transaction();

		byte* getBlock(uint64_t pos);
		const byte* getConstBlock(uint64_t pos);
		void commit();

	protected:
		void saveJournal();
		void clearJournal();

		Store* store_;
		File journalFile_;
		std::string journalFileName_;
		/**
		 * The true file size of the Store before a transaction has been opened
		 * (or the last time commit has been called). We don't need to journal
		 * any blocks that lie at or above this offset, because there was no
		 * actual data (Changes are rolled back simply by restoring the true
		 * file size, effectively truncating any newly written data)
		 */
		uint64_t preCommitStoreSize_;

		/**
		 * A mapping of file locations (which must be evenly divisible by 4K) to
		 * the 4-KB blocks where changes are staged until commit() or rollback()
		 * is called.
		 */
		std::unordered_map<uint64_t, TransactionBlock*> blocks_;

		/**
		 * A list of those TransactionBlocks that lie in the metadata portion
		 * of the store. In commit(), these are written to the store *after*
		 * all other blocks have been written, in order to prevent a data race
		 * by other processes that are accessing metadata (For example, we
		 * must set the page number of a tile in the Tile Index of a FeatureStore
		 * only once all of the actual tile data has been written to the Store.
		 */
		std::vector<TransactionBlock*> metadataBlocks_;
	};

private:
	LockLevel lock(LockLevel newLevel);
	bool tryExclusiveLock();
	
	static const uint64_t JOURNAL_END_MARKER = 0xffff'ffff'ffff'ffffUll;
	
	void checkJournal();
	std::string getJournalFileName() const
	{
		return fileName_ + ".journal";
	}
	bool isJournalValid(File& file);
	void applyJournal(File& file);

	std::string fileName_;
	int openMode_;
	LockLevel lockLevel_;
	FileLock lockRead_;
	FileLock lockWrite_;

	/**
	 * The currently open transaction, or nullptr if none.
	 */
	Transaction* transaction_;

	/**
	 * The mutex that must be held any time transaction_ is accessed.
	 */
	std::mutex transactionMutex_;

	friend class Transaction;
};


} // namespace clarisma
