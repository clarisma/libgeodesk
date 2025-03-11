// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <unordered_map>
#ifdef GEODESK_PYTHON
#include <Python.h>
#endif
#include <clarisma/store/BlobStore.h>
#include <clarisma/thread/ThreadPool.h>
#include <geodesk/export.h>
#include <geodesk/feature/Key.h>
#include <geodesk/feature/StringTable.h>
#include <geodesk/match/Matcher.h>
#include <geodesk/match/MatcherCompiler.h>
#include <geodesk/query/TileQueryTask.h>

class PyFeatures;       // not namespaced for now

namespace geodesk {

class MatcherHolder;

//  Possible threadpool alternatives:
//  - https://github.com/progschj/ThreadPool (Zlib license, header-only)

using clarisma::DataPtr;

/// @brief A Geographic Object Library.
///
/// This class if part of the **Low-Level API**. It is not intended to
/// be used directly by applications.
///
class GEODESK_API FeatureStore final : public clarisma::BlobStore
{
public:
    using IndexedKeyMap = std::unordered_map<uint16_t, uint16_t>;

    FeatureStore();
    ~FeatureStore() override;

    static FeatureStore* openSingle(std::string_view fileName);

    void open(const char* fileName)
    {
        BlobStore::open(fileName, 0);   // TODO: open mode
    }
    
    void addref()  { ++refcount_;  }
    void release() { if (--refcount_ == 0) delete this;  }
    size_t refcount() const { return refcount_; }

    DataPtr tileIndex() const
    { 
        return getPointer(TILE_INDEX_PTR_OFS);
    }

    uint32_t zoomLevels() const { return zoomLevels_; }
    StringTable& strings() { return strings_; }
    const IndexedKeyMap& keysToCategories() const { return keysToCategories_; }
    int getIndexCategory(int keyCode) const;
    const MatcherHolder* getMatcher(const char* query);

    const MatcherHolder* borrowAllMatcher() const { return &allMatcher_; }
    const MatcherHolder* getAllMatcher() 
    { 
        allMatcher_.addref();
        return &allMatcher_; 
    }
    bool isAllMatcher(const MatcherHolder* matcher) const
    {
        return matcher == &allMatcher_;
    }

    Key key(std::string_view k) const
    {
        int code = strings_.getCode(k.data(), k.size());
        return Key(k.data(), static_cast<uint32_t>(k.size()),
            code > TagValues::MAX_COMMON_KEY ? -1 : code);
    }

    #ifdef GEODESK_PYTHON
    PyObject* getEmptyTags();
    PyFeatures* getEmptyFeatures();
    #endif

    clarisma::ThreadPool<TileQueryTask>& executor() { return executor_; }

    DataPtr fetchTile(Tip tip);

protected:
    void initialize() override;

    DataPtr getPointer(int ofs) const
    {
        return DataPtr(mainMapping() + ofs).follow();
    }

private:
	static const uint32_t SubtypeMagic = 0x1CE50D6E;

    static const uint32_t ZOOM_LEVELS_OFS = 40;
    static const uint32_t TILE_INDEX_PTR_OFS = 44;
    static const uint32_t STRING_TABLE_PTR_OFS = 52;
    static const uint32_t INDEX_SCHEMA_PTR_OFS = 56;

    void readIndexSchema();

    void readTileSchema();

    static std::unordered_map<std::string, FeatureStore*>& getOpenStores();
    static std::mutex& getOpenStoresMutex();
    
    size_t refcount_;
    StringTable strings_;
    IndexedKeyMap keysToCategories_;
    MatcherCompiler matchers_;
    MatcherHolder allMatcher_;
    #ifdef GEODESK_PYTHON
    PyObject* emptyTags_;
    PyFeatures* emptyFeatures_;       
        // TODO: Need to keep this here due to #46, because a feature set
        // needs a valid reference to a FeatureStore (even if empty)
        // Not ideal, should have global singleton instead of per-store,
        // but PyFeatures requires a non-null MatcherHolder, which in turn
        // requires a FeatureStore
    #endif
    clarisma::ThreadPool<TileQueryTask> executor_;
    uint32_t zoomLevels_;
};


} // namespace geodesk
