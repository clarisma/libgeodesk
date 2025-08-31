// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/FeatureStore2_Transaction.h>
#include <geodesk/feature/TileIndexEntry.h>
#include <filesystem>
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

void FeatureStore2::Transaction::setup(const Metadata& metadata)
{
	// BlobStore::Transaction::setup();
	Header* header = header();
	header->versionHigh = VERSION_HIGH;
	header->versionLow = VERSION_LOW;
	header->flags = metadata.flags;
	header->guid = metadata.guid;
	header->snapshots[0].revision = metadata.revision;
	header->snapshots[0].revisionTimestamp = metadata.revisionTimestamp;
	header->settings = *metadata.settings;
	header->snapshots[0].tileCount = 0;

	// Place the Tile Index
	const size_t tileIndexSize = (*metadata.tileIndex + 1) * 4;
	const size_t tileIndexOfs = HEADER_BLOCK_SIZE;
	memcpy(mainMapping + tileIndexOfs, metadata.tileIndex, tileIndexSize);

	// Place the Indexed Keys Schema
	size_t indexedKeysOfs = tileIndexOfs + tileIndexSize;
	size_t indexedKeysSize = (*metadata.indexedKeys + 1) * 4;
	memcpy(mainMapping + indexedKeysOfs, metadata.indexedKeys, indexedKeysSize);

	// Place the Global String Table
	size_t stringTableOfs = indexedKeysOfs + indexedKeysSize;
	memcpy(mainMapping + stringTableOfs, metadata.stringTable, metadata.stringTableSize);

	// Place the Properties Table
	size_t propertiesOfs = stringTableOfs + metadata.stringTableSize;
	propertiesOfs += propertiesOfs & 1;
		// properties must be 2-byte aligned
	memcpy(mainMapping + propertiesOfs, metadata.properties, metadata.propertiesSize);

	size_t metadataSize = propertiesOfs + metadata.propertiesSize;

	header->tileIndexPtr = static_cast<int>(tileIndexOfs);
	header->indexSchemaPtr = static_cast<int>(indexedKeysOfs);
	header->stringTablePtr = static_cast<int>(stringTableOfs);
	header->propertiesPointer = static_cast<int>(propertiesOfs);
	tileIndexOfs_ = static_cast<uint32_t>(tileIndexOfs);
	setMetadataSize(header, metadataSize);

	store()->zoomLevels_ = ZoomLevels(header->settings.zoomLevels);
	store()->readIndexSchema(mainMapping + indexedKeysOfs);
		// TODO: This assumes we've written the metadata directly
		//  into the store, without journaling
		//  Block 0, which contains the pointer to the IndexSchema,
		//  has not been committed at this point, hence we need to
		//  explicitly pass the pointer
		// TODO: We should consolidate the store initialization?

	// TODO: We're not reading the string table, so this approach
	//  is inconsistent
	//  Idea: Since we only journal the root block, use common
	//  initialization that uses pointer to header (real or journaled)
	//  to perform initialization
}

} // namespace geodesk
