// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/libero/FreeStore_Journal.h>
#include "clarisma/data/BTreeSet.h"
#include "clarisma/data/HashMap.h"
#include "clarisma/data/HashSet.h"

namespace clarisma {

class FreeStore::Transaction
{
public:
	Transaction(FreeStore& store) :
		store_(store)
	{
	}


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
	void commit(bool isFinal = true);
	void end();

	uint32_t allocPages(uint32_t requestedPages);
	void freePages(uint32_t firstPage, uint32_t pages);
	void dumpFreeRanges();
	uint32_t addBlob(std::span<byte> data);

	void beginCreateStore();
	void endCreateStore();

	Header& header() noexcept { return header_; }

private:
	void buildFreeRangeIndex();
	void readFreeRangeIndex();
	void writeFreeRangeIndex();

	[[nodiscard]] bool isFirstPageOfSegment(uint32_t page) const
	{
		return (page & ((0x3fff'ffff) >> store_.pageSizeShift_)) == 0;
	}

	FreeStore& store_;
	Journal journal_;
	HashMap<uint64_t,const void*> editedBlocks_;
	BTreeSet<uint64_t> freeBySize_;
	BTreeSet<uint64_t> freeByStart_;
	Crc32C journalChecksum_;
	HeaderBlock header_;
};

} // namespace clarisma
