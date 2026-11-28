// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/FeatureStore.h>
#include <geodesk/match/MatcherCompiler.h>
#include <geodesk/feature/TileIndexEntry.h>
#include <filesystem>
#include <clarisma/io/FilePath.h>
#include <clarisma/util/log.h>
#include <clarisma/util/PbfDecoder.h>

#include "clarisma/util/Crc32C.h"
#ifdef GEODESK_PYTHON
#include "python/feature/PyTags.h"
#include "python/query/PyFeatures.h"
#include "python/util/util.h"
#endif

namespace geodesk {

using namespace clarisma;

// TODO: std::thread::hardware_concurrency() can return 0, so use
// a default value in that case (e.g. 4 threads)

// std::unordered_map<std::string, FeatureStore*> FeatureStore::openStores_;

FeatureStore::FeatureStore() :
    refcount_(1),
	#ifdef GEODESK_PYTHON
	emptyTags_(nullptr),
	emptyFeatures_(nullptr),
	#endif
	#ifdef NDEBUG
	executor_(std::thread::hardware_concurrency(), 0),
	#else
	executor_(1, 0),		// run single-threaded in debug mode
	#endif
	reverseTileIndex_(this)
{
}

FeatureStore* FeatureStore::openSingle(std::string_view relativeFileName)
{
	std::filesystem::path path;
	try
	{
		path = std::filesystem::canonical(
			(*FilePath::extension(relativeFileName) != 0) ? relativeFileName :
			std::string(relativeFileName) + ".gol");
	}
	catch (const std::filesystem::filesystem_error&)
	{
		throw FileNotFoundException(std::string(relativeFileName));
	}
	std::string fileName = path.string();

	// The try-block must enclose the lock of the openStores mutex
	// If creation of the new store fails, the FeatureStore object
	// is destroyed. But the FeatureStore destructor automatically
	// removes the store from the list of open stores (which requires
	// the mutex)

	FeatureStore* store = nullptr;
	try
	{
		std::lock_guard lock(getOpenStoresMutex());
		auto& openStores = getOpenStores();

		auto it = openStores.find(fileName);
		if (it != openStores.end())
		{
			store = it->second;
			store->addref();
			return store;
		}
		store = new FeatureStore();
		store->open(fileName.data());
		openStores[fileName] = store;
		return store;
	}
	catch (...)
	{
		if(store) delete store;
		throw;
	}
}

void FeatureStore::initialize(const byte* data)
{
	const Header* header = reinterpret_cast<const Header*>(data);
	if (header->magic != MAGIC)
	{
		throw FreeStoreException(header->magic == MAGIC_GOB ?
			"Geo-Object Bundle; import it first using \"gol load\"" :
			"Not a Geographic Object Library");
	}
	// TODO: Version check

	const SnapshotData* snapshot = &header->snapshots[header->activeSnapshot];
	tileIndex_ =
		const_cast<uint32_t*>(		// TODO
			reinterpret_cast<const uint32_t*>(data + offsetOfPage(snapshot->tileIndex)));

	snapshot_.initStrings(reinterpret_cast<const uint8_t*>(
		data + header->stringTablePtr));
	zoomLevels_ = ZoomLevels(header->settings.zoomLevels);
	snapshot_.readIndexSchema(data + header->indexSchemaPtr);
}

FeatureStore::~FeatureStore()
{
	// LOG("Destroying FeatureStore...");
	#ifdef GEODESK_PYTHON
	Py_XDECREF(emptyTags_);
	Py_XDECREF(emptyFeatures_);
	#endif
	// LOG("Destroyed FeatureStore.");

	std::lock_guard lock(getOpenStoresMutex());
	auto& openStores = getOpenStores();
	openStores.erase(fileName());
}


TilePtr FeatureStore::fetchTile(Tip tip) const
{
	TileIndexEntry entry(tileIndex_[tip]);
	if(!entry.isLoadedAndCurrent())	[[unlikely]]
	{
		return TilePtr();
	}
	return TilePtr(pagePointer(entry.page()));
}


// TODO: inefficient, should store string table size when initializing
std::span<byte> FeatureStore::stringTableData() const
{
	DataPtr pTable (data() + header()->stringTablePtr);
	int count = pTable.getUnsignedShort();
	byte* p = pTable.bytePtr();
	p += 2;
	for(int i=0; i<count; i++)
	{
		p += reinterpret_cast<ShortVarString*>(p)->totalSize();
	}
	return { pTable.bytePtr(), static_cast<size_t>(p - pTable.bytePtr()) };
}

std::span<byte> FeatureStore::propertiesData() const
{
	DataPtr pTable (data() + header()->propertiesPtr);
	int count = pTable.getUnsignedShort();
	byte* p = pTable.bytePtr();
	p += 2;
	for(int i=0; i<count * 2; i++)
	{
		p += reinterpret_cast<ShortVarString*>(p)->totalSize();
	}
	return { pTable.bytePtr(), static_cast<size_t>(p - pTable.bytePtr()) };
}

const MatcherHolder* FeatureStore::getMatcher(const char* query) const
{
	return MatcherCompiler(snapshot()).getMatcher(query);
}

HashMap<std::string, FeatureStore*>& FeatureStore::getOpenStores()
{
	static HashMap<std::string, FeatureStore*> openStores;
	return openStores;
}

std::mutex& FeatureStore::getOpenStoresMutex()
{
	static std::mutex openStoresMutex;
	return openStoresMutex;
}

bool FeatureStore::isTileValid(const byte* pTile)
{
	DataPtr p(pTile);
	Crc32C checksum;
	uint32_t payloadSize = p.getUnsignedInt();
	checksum.update(p.ptr(), payloadSize);
	return checksum.get() == (p + payloadSize).getUnsignedIntUnaligned();
}

void FeatureStore::gatherUsedRanges(std::vector<uint64_t>& ranges)
{
	// TODO
}

} // namespace geodesk

