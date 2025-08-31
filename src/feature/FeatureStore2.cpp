// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/FeatureStore2.h>
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

FeatureStore2::FeatureStore2() :
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

FeatureStore2* FeatureStore2::openSingle(std::string_view relativeFileName)
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

	FeatureStore2* store = nullptr;
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
		store = new FeatureStore2();
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

void FeatureStore2::initialize(bool create)
{
	FreeStore::initialize(create);

	strings_.create(reinterpret_cast<const uint8_t*>(
		mainMapping() + header()->stringTablePtr));
	zoomLevels_ = ZoomLevels(header()->settings.zoomLevels);
	readIndexSchema(mainMapping() + header()->indexSchemaPtr);
}

FeatureStore2::~FeatureStore2()
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

// TODO: Return TilePtr?
DataPtr FeatureStore2::fetchTile(Tip tip) const
{
	TileIndexEntry entry(tileIndex_[tip]);
	if(!entry.isLoadedAndCurrent())	[[unlikely]]
	{
		return DataPtr();
	}
	return pagePointer(entry.page());
}



void FeatureStore2::readIndexSchema(DataPtr p)
{
	int32_t count = p.getInt();
	keysToCategories_.reserve(count);
	for (int i = 0; i < count; i++)
	{
		p += 4;
		keysToCategories_.insert({ p.getUnsignedShort(), (p+2).getUnsignedShort() });
	}
}

int FeatureStore2::getIndexCategory(int keyCode) const
{
	auto it = keysToCategories_.find(keyCode);
	if (it != keysToCategories_.end())
	{
		return it->second;
	}
	return 0;
}

std::vector<std::string_view> FeatureStore2::indexedKeyStrings() const
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
std::span<byte> FeatureStore2::stringTableData() const
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

std::span<byte> FeatureStore2::propertiesData() const
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

const MatcherHolder* FeatureStore2::getMatcher(const char* query)
{
	return matchers_.getMatcher(query);
}

#ifdef GEODESK_PYTHON

PyObject* FeatureStore2::getEmptyTags()
{
	if (!emptyTags_)
	{
		emptyTags_ = PyTags::create(this, TagTablePtr::empty());
		if (!emptyTags_) return NULL;
	}
	return Python::newRef(emptyTags_);
}

PyFeatures* FeatureStore2::getEmptyFeatures()
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


std::unordered_map<std::string, FeatureStore2*>& FeatureStore2::getOpenStores()
{
	static std::unordered_map<std::string, FeatureStore2*> openStores;
	return openStores;
}

std::mutex& FeatureStore2::getOpenStoresMutex()
{
	static std::mutex openStoresMutex;
	return openStoresMutex;
}

} // namespace geodesk
