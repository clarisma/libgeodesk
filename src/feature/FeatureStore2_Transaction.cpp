// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef GEODESK_LIBERO
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

void FeatureStore::Transaction::setup(const Metadata& metadata)
{
	// BlobStore::Transaction::setup();
	Header& header = Transaction::header();
	header.versionHigh = VERSION_HIGH;
	header.versionLow = VERSION_LOW;
	header.flags = metadata.flags;
	header.guid = metadata.guid;
	header.snapshots[0].revision = metadata.revision;
	header.snapshots[0].revisionTimestamp = metadata.revisionTimestamp;
	header.settings = *metadata.settings;
	header.snapshots[0].tileCount = 0;

	// Write the Indexed Keys Schema
	size_t indexedKeysOfs = BLOCK_SIZE;
	size_t indexedKeysSize = (*metadata.indexedKeys + 1) * 4;
	file().writeAllAt(indexedKeysOfs, metadata.indexedKeys, indexedKeysSize);

	// Place the Global String Table
	size_t stringTableOfs = indexedKeysOfs + indexedKeysSize;
	file().writeAllAt(stringTableOfs, metadata.stringTable, metadata.stringTableSize);

	// Place the Properties Table
	size_t propertiesOfs = stringTableOfs + metadata.stringTableSize;
	propertiesOfs += propertiesOfs & 1;
		// properties must be 2-byte aligned
	file().writeAllAt(propertiesOfs, metadata.properties, metadata.propertiesSize);

	size_t metaSectionSize = propertiesOfs + metadata.propertiesSize;

	header.indexSchemaPtr = static_cast<int>(indexedKeysOfs);
	header.stringTablePtr = static_cast<int>(stringTableOfs);
	header.propertiesPtr = static_cast<int>(propertiesOfs);
	header.metaSectionSize = static_cast<uint32_t>(metaSectionSize);

	store().zoomLevels_ = ZoomLevels(header.settings.zoomLevels);
	store().readIndexSchema(reinterpret_cast<const byte*>(metadata.indexedKeys));

	// TODO: We're not reading the string table, so this approach
	//  is inconsistent

}

} // namespace geodesk

#endif