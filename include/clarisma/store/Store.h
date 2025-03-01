// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <unordered_map>
#include <clarisma/io/FileLock.h>
#include <clarisma/io/ExpandableMappedFile.h>
#include <clarisma/util/DateTime.h>
#include <clarisma/util/MutableDataPtr.h>


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
	uint64_t allocatedSize() const
	{
		return File::allocatedSize();
	}

	bool isBlank() const;

	enum LockLevel
	{
		LOCK_NONE = 0,
		LOCK_READ = 1,
		LOCK_APPEND = 2,
		LOCK_EXCLUSIVE = 3
	};

protected:
	virtual void initialize(bool create) = 0;
	virtual DateTime getLocalCreationTimestamp() const = 0;
	virtual uint64_t getTrueSize() const = 0;
	// void* mapSegment(uint32_t segNumber, uint32_t segCount);

	byte* data(uint64_t ofs)
	{
		return translate(ofs);
	}

	void error(const char* msg) const;

	class JournaledBlock
	{
	public:
		explicit JournaledBlock(byte* original) :
			original_(original),
			next_(nullptr)
		{
			memcpy(current_, original, SIZE);
		}
		[[nodiscard]] byte* original() const { return original_; }
		[[nodiscard]] byte* current() { return current_; }

		static constexpr int SIZE = 4096;

	private:
		byte* original_;
		JournaledBlock* next_;
		byte current_[SIZE];
	};

	using JournaledBlocks = std::unordered_map<uint64_t, std::unique_ptr<JournaledBlock>>;

	class Journal : public File
	{
	public:
		Journal() = default;

		void setFileName(const std::string& fileName)
		{
			fileName_ = fileName;
		}

		bool exists() const
		{
			return File::exists(fileName_.c_str());
		}

		void open(int openMode)
		{
			File::open(fileName_.c_str(), openMode);
		}

		void remove() const
		{
			assert(!isOpen());
			File::remove(fileName_.c_str());
		}

		void save(DateTime timestamp, const JournaledBlocks& blocks);
		uint32_t readInstruction();
		bool isValid(DateTime storeCreationTimestamp);
		void apply(byte* storeData, size_t storeSize);
		void clear();

	private:
		static constexpr  uint64_t JOURNAL_END_MARKER = 0xffff'ffff'ffff'ffffUll;

		std::string fileName_;
	};

	class Transaction
	{
	public:
		enum JournalMode
		{
			NONE = 0,				// don't use a journal
			INVALIDATE = 1,			// declare entire store invalid if xaction fails
			ROLLBACK = 2			// undo the xaction if it fails (default)
		};

		explicit Transaction(Store* store);
		~Transaction() { if(isOpen_) end(); };

		void begin(LockLevel lockLevel);
		byte* getBlock(uint64_t pos);
		const byte* getConstBlock(uint64_t pos);
		MutableDataPtr dataPtr(uint64_t pos);
		void commit();
		void end();

	protected:
		void saveJournal();
		void clearJournal();

		Store* store_;
		/**
		 * The true file size of the Store before a transaction has been opened
		 * (or the last time commit has been called). We don't need to journal
		 * any blocks that lie at or above this offset, because there was no
		 * actual data (Changes are rolled back simply by restoring the true
		 * file size, effectively truncating any newly written data)
		 */
		uint64_t preCommitStoreSize_;
		LockLevel preTransactionLockLevel_;
		bool isOpen_;

		/**
		 * A mapping of file locations (which must be evenly divisible by 4K) to
		 * the 4-KB blocks where changes are staged until commit() or rollback()
		 * is called.
		 */
		 JournaledBlocks blocks_;

		/**
		 * A list of those TransactionBlocks that lie in the metadata portion
		 * of the store. In commit(), these are written to the store *after*
		 * all other blocks have been written, in order to prevent a data race
		 * by other processes that are accessing metadata (For example, we
		 * must set the page number of a tile in the Tile Index of a FeatureStore
		 * only once all of the actual tile data has been written to the Store.
		 */
		JournaledBlock* firstRegularBlock_;
		JournaledBlock* firstMetadataBlock_;
	};

private:
	LockLevel lock(LockLevel newLevel);
	bool tryExclusiveLock();
	
	static constexpr uint64_t JOURNAL_END_MARKER = 0xffff'ffff'ffff'ffffUll;
	
	void processJournal();
	void applyJournal();

	std::string fileName_;
	int openMode_;
	LockLevel lockLevel_;
	FileLock lockRead_;
	FileLock lockWrite_;
	Journal journal_;

	/**
	 * The currently open transaction, or nullptr if none.
	 */
	Transaction* transaction_;

	/**
	 * The mutex that must be held any time transaction_ is accessed.
	 */
	std::mutex transactionMutex_;
	// std::thread::id transactionThread_;

	friend class Transaction;
};

} // namespace clarisma
