// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/FeatureStore.h>
#include <geodesk/feature/TileIndexEntry.h>
#include <filesystem>
#include <clarisma/io/FilePath.h>
#include <clarisma/util/log.h>
#include <clarisma/util/PbfDecoder.h>
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
	matchers_(this),	// TODO: this not initialized yet!
	#ifdef GEODESK_PYTHON
	emptyTags_(nullptr),
	emptyFeatures_(nullptr),
	#endif
	#ifdef NDEBUG
	executor_(std::thread::hardware_concurrency(), 0)
	#else
	executor_(1, 0)		// run single-threaded in debug mode
	#endif
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
		auto openStores = getOpenStores();

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

void FeatureStore::initialize(bool create)
{
	BlobStore::initialize(create);

	strings_.create(reinterpret_cast<const uint8_t*>(
		mainMapping() + header()->stringTablePtr));
	zoomLevels_ = ZoomLevels(header()->settings.zoomLevels);
	readIndexSchema(mainMapping() + header()->indexSchemaPtr);
}

FeatureStore::~FeatureStore()
{
	LOG("Destroying FeatureStore...");
	#ifdef GEODESK_PYTHON
	Py_XDECREF(emptyTags_);
	Py_XDECREF(emptyFeatures_);
	#endif
	LOG("Destroyed FeatureStore.");

	std::lock_guard lock(getOpenStoresMutex());
	auto openStores = getOpenStores();
	openStores.erase(fileName());
}

// TODO: Return TilePtr
DataPtr FeatureStore::fetchTile(Tip tip)
{
	TileIndexEntry entry((tileIndex() + (tip * 4)).getUnsignedInt());
	if(!entry.isLoadedAndCurrent())	[[unlikely]]
	{
		return DataPtr();
		// TODO: load tiles?
	}
	return pagePointer(entry.page());
}



void FeatureStore::readIndexSchema(DataPtr p)
{
	int32_t count = p.getInt();
	keysToCategories_.reserve(count);
	for (int i = 0; i < count; i++)
	{
		p += 4;
		keysToCategories_.insert({ p.getUnsignedShort(), (p+2).getUnsignedShort() });
	}
}

int FeatureStore::getIndexCategory(int keyCode) const
{
	auto it = keysToCategories_.find(keyCode);
	if (it != keysToCategories_.end())
	{
		return it->second;
	}
	return 0;
}

std::vector<std::string_view> FeatureStore::indexedKeyStrings() const
{
	std::vector<std::string_view> keys;
	keys.reserve(keysToCategories_.size());
	for(auto& entry: keysToCategories_)
	{
		keys.emplace_back(strings_.getGlobalString(entry.first)->toStringView());
	}
	return keys;
}

// TODO: inefficient, should store strign table size when initializing
std::span<byte> FeatureStore::stringTableData() const
{
	DataPtr pTable (mainMapping() + header()->stringTablePtr);
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
	DataPtr pTable (mainMapping() + header()->propertiesPointer);
	int count = pTable.getUnsignedShort();
	byte* p = pTable.bytePtr();
	p += 2;
	for(int i=0; i<count * 2; i++)
	{
		p += reinterpret_cast<ShortVarString*>(p)->totalSize();
	}
	return { pTable.bytePtr(), static_cast<size_t>(p - pTable.bytePtr()) };
}

const MatcherHolder* FeatureStore::getMatcher(const char* query)
{
	return matchers_.getMatcher(query);
}

#ifdef GEODESK_PYTHON

PyObject* FeatureStore::getEmptyTags()
{
	if (!emptyTags_)
	{
		emptyTags_ = PyTags::create(this, TagTablePtr::empty());
		if (!emptyTags_) return NULL;
	}
	return Python::newRef(emptyTags_);
}

PyFeatures* FeatureStore::getEmptyFeatures()
{
	if (!emptyFeatures_)
	{
		allMatcher_.addref();
		emptyFeatures_ = PyFeatures::createEmpty(this, &allMatcher_);
	}
	Py_INCREF(emptyFeatures_);
	return emptyFeatures_;
}

#endif


std::unordered_map<std::string, FeatureStore*>& FeatureStore::getOpenStores()
{
	static std::unordered_map<std::string, FeatureStore*> openStores;
	return openStores;
}

std::mutex& FeatureStore::getOpenStoresMutex()
{
	static std::mutex openStoresMutex;
	return openStoresMutex;
}


void FeatureStore::Transaction::addTile(Tip tip, ByteSpan data)
{
	PageNum page = addBlob(data);
	MutableDataPtr ptr = dataPtr(tileIndexOfs_ + tip * 4);
	assert(ptr.getUnsignedInt() == 0);
	ptr.putUnsignedInt(TileIndexEntry(page, TileIndexEntry::CURRENT));
	getHeaderBlock()->tileCount++;
	LOGS << "TileCount is now " << getHeaderBlock()->tileCount << "\n";
}


void FeatureStore::Transaction::setup(const Metadata& metadata)
{
	BlobStore::Transaction::setup();
	Header* header = getHeaderBlock();
	header->subtypeMagic = SUBTYPE_MAGIC;
	header->subtypeVersionHigh = 2;
	header->subtypeVersionLow = 0;
	header->flags = metadata.flags;
	header->guid = metadata.guid;
	header->revision = metadata.revision;
	header->revisionTimestamp = metadata.revisionTimestamp;
	header->settings = *metadata.settings;
	header->tileCount = 0;

	// Place the Tile Index
	byte* mainMapping = store()->mainMapping();
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
