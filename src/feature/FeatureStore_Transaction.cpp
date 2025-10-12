// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/FeatureStore_Transaction.h>
#include <geodesk/feature/TileIndexEntry.h>
#include <filesystem>
#include <random>
#include <clarisma/io/FilePath.h>
#include <clarisma/util/log.h>
#include <clarisma/util/PbfDecoder.h>


namespace geodesk {

using namespace clarisma;

/*
void FeatureStore2::Transaction::addTile(Tip tip, std::span<byte> data)
{
	uint32_t page = addBlob(data);
	MutableDataPtr ptr = dataPtr(tileIndexOfs_ + tip * 4);
	assert(ptr.getUnsignedInt() == 0);
	ptr.putUnsignedInt(TileIndexEntry(page, TileIndexEntry::CURRENT));
	getHeaderBlock()->tileCount++;
	LOGS << "TileCount is now " << getHeaderBlock()->tileCount << "\n";
}
*/

void FeatureStore::Transaction::setup(
	const Metadata& metadata,
	std::unique_ptr<uint32_t[]>&& tileIndex)
{
	// BlobStore::Transaction::setup();
	Header& header = Transaction::header();
	header.magic = MAGIC;
	header.versionHigh = VERSION_HIGH;
	header.versionLow = VERSION_LOW;

	// Initialize commitId with a random 64-bit value
	std::random_device rd;
	std::mt19937_64 gen(rd());  // 64-bit Mersenne Twister
	std::uniform_int_distribution<uint64_t> dist(
		0, std::numeric_limits<uint64_t>::max());
	header.commitId = dist(gen);

	header.flags = metadata.flags;
	header.guid = metadata.guid;
	header.tipCount = tileIndex[0] / 4;
	header.tileIndexChecksum = Crc32C::compute(tileIndex.get(), tileIndex[0] + 4);
	header.snapshots[0].revision = metadata.revision;
	header.snapshots[0].revisionTimestamp = metadata.revisionTimestamp;
	header.settings = *metadata.settings;
	header.snapshots[0].tileCount = 0;

	tileIndex_ = std::move(tileIndex);

	Crc32C metadataChecksum;

	// Write the Indexed Keys Schema
	size_t indexedKeysOfs = BLOCK_SIZE;
	size_t indexedKeysSize = (*metadata.indexedKeys + 1) * 4;
	file().writeAllAt(indexedKeysOfs, metadata.indexedKeys, indexedKeysSize);
	metadataChecksum.update(metadata.indexedKeys, indexedKeysSize);

	// Place the Global String Table
	size_t stringTableOfs = indexedKeysOfs + indexedKeysSize;
	file().writeAllAt(stringTableOfs, metadata.stringTable, metadata.stringTableSize);
	metadataChecksum.update(metadata.stringTable, metadata.stringTableSize);

	// Place the Properties Table
	size_t propertiesOfs = stringTableOfs + metadata.stringTableSize;

	/*
	 // TODO: We're foregoing alignment for now to simplify
	 //  the checksum calculation of the metadata
	propertiesOfs += propertiesOfs & 1;
		// properties must be 2-byte aligned
	*/
	file().writeAllAt(propertiesOfs, metadata.properties, metadata.propertiesSize);
	metadataChecksum.update(metadata.properties, metadata.propertiesSize);

	size_t metaSectionSize = propertiesOfs + metadata.propertiesSize - BLOCK_SIZE;

	header.indexSchemaPtr = static_cast<int>(indexedKeysOfs);
	header.stringTablePtr = static_cast<int>(stringTableOfs);
	header.propertiesPtr = static_cast<int>(propertiesOfs);
	setMetaSectionSize(static_cast<uint32_t>(metaSectionSize));

	header.metadataChecksum = metadataChecksum.get();

	store().zoomLevels_ = ZoomLevels(header.settings.zoomLevels);
	store().readIndexSchema(reinterpret_cast<const byte*>(metadata.indexedKeys));

	// TODO: We're not reading the string table, so this approach
	//  is inconsistent

}

void FeatureStore::Transaction::putTile(Tip tip, std::span<const uint8_t> data)
{
	// TODO: Free existing tile

	TileIndexEntry prevEntry(tileIndex_[tip]);
	uint32_t page = addBlob(data);
	tileIndex_[tip] = TileIndexEntry(page, TileIndexEntry::CURRENT);

	// TODO: use modified snapshot instead of active snapshot
	//  once we support concurrent updates
	header().snapshots[header().activeSnapshot].tileCount +=
		static_cast<uint32_t>(!prevEntry.isLoadedAndCurrent());
}


void FeatureStore::Transaction::begin()
{
	FreeStore::Transaction::begin();
	Snapshot& activeSnapshot = header().snapshots[header().activeSnapshot];
	uint32_t tileIndexPage = activeSnapshot.tileIndex;
	if (tileIndexPage)	[[likely]]
	{
		uint32_t tipCount = header().tipCount;
		uint32_t tileIndexSlotCount = tipCount + 1;
		tileIndex_.reset(new uint32_t[tileIndexSlotCount]);
		memcpy(tileIndex_.get(),
			store().data() + store().offsetOfPage(tileIndexPage),
			tileIndexSlotCount * 4);
	}
}

void FeatureStore::Transaction::commit(bool isFinal)
{
	// TODO: Currently, we're writing exclusively, so Snapshot 0
	//  is always active; update this once we support concurrent writes

	Snapshot& activeSnapshot = header().snapshots[header().activeSnapshot];
	uint32_t tileIndexSize = tileIndex_[0] + 4;
	if (activeSnapshot.tileIndex)	[[likely]]
	{
		freePages(activeSnapshot.tileIndex,
			store().pagesForBytes(tileIndexSize));
	}
	activeSnapshot.tileIndex = addBlob(
		{reinterpret_cast<uint8_t*>(tileIndex_.get()), tileIndexSize});
	FreeStore::Transaction::commit(isFinal);
}


} // namespace geodesk

