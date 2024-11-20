// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/FeatureStore_v2.h>
#include <filesystem>
#include <clarisma/util/log.h>
#include <clarisma/util/PbfDecoder.h>
#ifdef GEODESK_PYTHON
#include "python/feature/PyTags.h"
#include "python/query/PyFeatures.h"
#include "python/util/util.h"
#endif

namespace geodesk::v2 {

using namespace clarisma;

// TODO: std::thread::hardware_concurrency() can return 0, so use
// a default value in that case (e.g. 4 threads)

// std::unordered_map<std::string, FeatureStore*> FeatureStore::openStores_;

FeatureStore::FeatureStore()
  : refcount_(1),
	// matchers_(this),	// TODO: this not initialized yet!
		// TOD: re-enable
	#ifdef GEODESK_PYTHON
	emptyTags_(nullptr),
	emptyFeatures_(nullptr),
	#endif
	executor_(/* 1 */ std::thread::hardware_concurrency(), 0)  // TODO: disabled for testing
{
}

FeatureStore* FeatureStore::openSingle(std::string_view relativeFileName)
{
	std::filesystem::path path;
	try
	{
		path = std::filesystem::canonical(
			(*File::extension(relativeFileName) != 0) ? relativeFileName :
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

void FeatureStore::initialize()
{
	strings_.create(reinterpret_cast<const uint8_t*>(
		mainMapping() + header()->stringTablePtr));
	zoomLevels_ = header()->zoomLevels;
	readIndexSchema();
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
	uint32_t pageEntry = (tileIndex() + (tip * 4)).getUnsignedInt();
	// Bit 0 is a flag bit (page vs. child pointer)
	// TODO: load tiles

	return pagePointer(pageEntry >> 1);
}



void FeatureStore::readIndexSchema()
{
	DataPtr p(mainMapping() + header()->indexSchemaPtr);
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

/* // TODO: re-enable
const MatcherHolder* FeatureStore::getMatcher(const char* query)
{
	return matchers_.getMatcher(query);
}
*/

#ifdef GEODESK_PYTHON

// TODO: re-enable
/*
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
*/

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
}


void FeatureStore::CreateTransaction::begin(const char* filename,
	ZoomLevels zoomLevels, const uint32_t* tileIndex,
	const uint8_t* stringTable, size_t stringTableSize)
{
	BlobStore::CreateTransaction<FeatureStore>::begin(filename);
	Header* header = reinterpret_cast<Header*>(getRootBlock());
	header->subtypeMagic = SUBTYPE_MAGIC;
	header->subtypeVersionHigh = 2;
	header->subtypeVersionLow = 0;
	header->zoomLevels = static_cast<uint16_t>(zoomLevels);

	byte* mainMapping = store_.mainMapping();
	size_t tileIndexSize = (*tileIndex + 1) * 4;
	size_t tileIndexOfs = HEADER_BLOCK_SIZE;
	memcpy(mainMapping + tileIndexOfs, tileIndex, tileIndexSize);
	size_t stringTableOfs = HEADER_BLOCK_SIZE + tileIndexSize;
	memcpy(mainMapping + stringTableOfs, stringTable, stringTableSize);

	header->tileIndexPtr = static_cast<int>(tileIndexOfs);
	header->stringTablePtr = static_cast<int>(stringTableOfs);
	header->tileIndexPtr = static_cast<int>(tileIndexOfs);
	tileIndexOfs_ = static_cast<uint32_t>(tileIndexOfs);
	setMetadataSize(header, HEADER_BLOCK_SIZE +
		tileIndexSize + stringTableSize);
}

} // namespace geodesk