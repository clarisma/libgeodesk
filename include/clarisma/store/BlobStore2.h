// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/store/Store.h>
#include <map>
#include <unordered_map>
#include <clarisma/data/Span.h>
#include <clarisma/util/DataPtr.h>

using std::byte;

namespace clarisma {

class BlobStore2 : public Store
{
public:
	using Store::LockLevel;

	struct Header		// NOLINT: No constructor needed
	{
		uint32_t magic;
		uint16_t versionLow;
		uint16_t versionHigh;
		DateTime creationTimestamp;
		uint8_t  pageSize;
		uint8_t  reserved[3];
		uint32_t metadataSize;
		uint32_t reserved2;
		/**
		 * The total number of pages in use (includes free blobs and metadata pages).
		 */
		uint32_t totalPageCount;
		uint32_t firstFreeMetaPage;
		uint32_t freeSizeTreeRoot;
		uint32_t freeEndTreeRoot;
		uint32_t reserved3;
	};

	static_assert(sizeof(Header) == 48, "Expected 48-byte Header");

	class Transaction : protected Store::Transaction
	{
	public:
		explicit Transaction(BlobStore2* store) :
			Store::Transaction(store)
		{
		}

		void begin(LockLevel lockLevel = LockLevel::LOCK_APPEND)
		{
			Store::Transaction::begin(lockLevel);
		}
		[[nodiscard]] BlobStore2* store() const { return reinterpret_cast<BlobStore2*>(store_); }

		void beginCreateStore();
		void endCreateStore();

		uint32_t allocPages(uint32_t requestedPages);

		void freePages(uint32_t firstPage, uint32_t pages)
		{
			freedBlobs_.emplace_back(firstPage, pages);
		}

		void commit();
		void end() { Store::Transaction::end(); }

	protected:
		Header* getRootBlock()
		{
			return reinterpret_cast<Header*>(getBlock(0));
		}

	private:
		byte* getPageBlock(uint32_t page)
		{
			return getBlock(static_cast<uint64_t>(page)
					<< store()->pageSizeShift_);
		}

		uint32_t allocMetaPage();
		void freeMetaPage(uint32_t page);
		void performFreePages(uint32_t firstPage, uint32_t pages);
		[[nodiscard]] bool isFirstPageOfSegment(uint32_t page) const
		{
			return (page & ((0x3fff'ffff) >> store()->pageSizeShift_)) == 0;
		}

		struct FreedBlob
        {
			uint32_t firstPage;
			uint32_t pages;
		};

		std::vector<FreedBlob> freedBlobs_;

		friend class FreeTree;
	};

	uint64_t offsetOf(uint32_t page) const
	{
		return static_cast<uint64_t>(page) << pageSizeShift_;
	}

	byte* translatePage(uint32_t page)
	{
		return translate(offsetOf(page));
	}

protected:
	static const uint32_t MAGIC = 0x7ADA0BB1;

	static const int BLOB_HEADER_SIZE = 8;

	DataPtr pagePointer(uint32_t page)
	{
		return DataPtr(data(static_cast<uint64_t>(page) << pageSizeShift_));
	}

	void initialize(bool create) override;

	Header* getRoot() const
	{
		return reinterpret_cast<Header*>(mainMapping());
	}

	DateTime getLocalCreationTimestamp() const override;
	uint64_t getTrueSize() const override;
	uint32_t pagesForPayloadSize(uint32_t payloadSize) const;
	static void setMetadataSize(Header* header, size_t size);

private:
	static void initHeader(Header* header);

	uint32_t pageSizeShift_ = 12;	// TODO: default 4KB page

	template<typename T>
	friend class CreateTransaction;
};

} // namespace clarisma
