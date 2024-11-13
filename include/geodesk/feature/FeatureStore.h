// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <span>
#include <unordered_map>
#ifdef GEODESK_PYTHON
#include <Python.h>
#endif
#include <clarisma/store/BlobStore.h>
#include <clarisma/thread/ThreadPool.h>
#include <clarisma/util/DateTime.h>
#include <clarisma/util/UUID.h>
#include <geodesk/export.h>
#include <geodesk/feature/StringTable.h>
#include <geodesk/feature/ZoomLevels.h>
#include <geodesk/match/Matcher.h>
#include <geodesk/match/MatcherCompiler.h>
#include <geodesk/query/TileQueryTask.h>

// \cond

class PyFeatures;       // not namespaced for now

namespace geodesk {

class MatcherHolder;

//  Possible threadpool alternatives:
//  - https://github.com/progschj/ThreadPool (Zlib license, header-only)

using std::byte;
using clarisma::DataPtr;
using clarisma::DateTime;



class GEODESK_API FeatureStore final : public clarisma::BlobStore
{
public:
    using IndexedKeyMap = std::unordered_map<uint16_t, uint16_t>;

    struct IndexSettings
    {
        uint16_t    rtreeBranchSize;
        uint8_t     rtreeAlgo;
        uint8_t     maxKeyIndexes;
        uint32_t    keyIndexMinFeatures;
    };

    struct Header : BlobStore::Header
    {
        uint32_t subtypeMagic;
        uint16_t subtypeVersionHigh;
        uint16_t subtypeVersionLow;
        uint16_t zoomLevels;
        uint16_t flags;
        int32_t tileIndexPtr;
        int32_t stringTablePtr;
        int32_t indexSchemaPtr;
        clarisma::UUID guid;
        uint32_t revision;
        uint32_t modifiedSinceRevision;
        DateTime revisionTimestamp;
        DateTime modifiedSinceTimestamp;
        uint32_t sourceReplicationNumber;
    };

    using Store::LockLevel;
    using Store::OpenMode;

    FeatureStore();
    ~FeatureStore() override;

    static FeatureStore* openSingle(std::string_view fileName);

    void open(const char* fileName, int openMode = 0)
    {
        BlobStore::open(fileName, openMode);
    }

    void addref()  { ++refcount_;  }
    void release() { if (--refcount_ == 0) delete this;  }
    size_t refcount() const { return refcount_; }

    DataPtr tileIndex() const noexcept
    { 
        return { mainMapping() + header()->tileIndexPtr };
    }

    const clarisma::UUID& guid() const { return header()->guid; }
    uint32_t revision() const { return header()->revision; }
    DateTime revisionTimestamp() const { return header()->revisionTimestamp; }

    ZoomLevels zoomLevels() const { return zoomLevels_; }
    StringTable& strings() { return strings_; }
    const IndexedKeyMap& keysToCategories() const { return keysToCategories_; }
    int getIndexCategory(int keyCode) const;
    const Header* header() const
    {
        return reinterpret_cast<Header*>(mainMapping());
    }
    std::span<byte> stringTableData() const;
    std::span<byte> propertiesData() const;
    using BlobStore::properties;

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

    #ifdef GEODESK_PYTHON
    PyObject* getEmptyTags();
    PyFeatures* getEmptyFeatures();
    #endif

    clarisma::ThreadPool<TileQueryTask>& executor() { return executor_; }

    DataPtr fetchTile(Tip tip);

    struct Metadata
    {
        Metadata(const clarisma::UUID& guid_, uint32_t rev) :
            guid(guid_), revision(rev), tileIndex(nullptr),
            stringTable(nullptr), stringTableSize(0),
            properties(nullptr), propertiesSize(0),
            indexedKeys(nullptr)
        {
        }

        clarisma::UUID guid;
        uint32_t revision;
        DateTime revisionTimestamp;
        const uint32_t* tileIndex;
        const uint8_t* stringTable;
        size_t stringTableSize;
        const uint8_t* properties;
        size_t propertiesSize;
        const uint32_t* indexedKeys;
    };

    class Transaction : public BlobStore::Transaction
    {
    public:
        explicit Transaction(FeatureStore* store) :
            BlobStore::Transaction(store),
            tileIndexOfs_(0) {}

        void begin(LockLevel lockLevel = LockLevel::LOCK_APPEND)
        {
            BlobStore::Transaction::begin(lockLevel);
            const Header* header = store()->header();
            tileIndexOfs_ = header->tileIndexPtr + offsetof(Header, tileIndexPtr);
        }

        FeatureStore* store() const
        {
            return reinterpret_cast<FeatureStore*>(store_);
        }

        // TODO: zoom level, other settings
        void setup(const Metadata& metadata);

        void addTile(Tip tip, clarisma::ByteSpan data);

    protected:
        uint32_t tileIndexOfs_;
    };

protected:
    void initialize(bool create) override;

    DataPtr getPointer(int ofs) const
    {
        return DataPtr(mainMapping() + ofs).follow();
    }

private:
	static constexpr uint32_t SUBTYPE_MAGIC = 0x1CE50D6E;

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
    ZoomLevels zoomLevels_;

    friend class Transaction;
};

} // namespace geodesk

// \endcond