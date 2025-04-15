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
#include <geodesk/feature/Key.h>
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



/// @brief A Geographic Object Library.
///
/// This class if part of the **Low-Level API**. It is not intended to
/// be used directly by applications.
///
class GEODESK_API FeatureStore final : public clarisma::BlobStore
{
public:
    using IndexedKeyMap = std::unordered_map<uint16_t, uint16_t>;

    struct Settings
    {
        uint16_t    zoomLevels;
        uint16_t    reserved;
        uint16_t    rtreeBranchSize;
        uint8_t     rtreeAlgo;
        uint8_t     maxKeyIndexes;
        uint32_t    keyIndexMinFeatures;
    };

    struct Header : BlobStore::Header
    {
        enum Flags
        {
            WAYNODE_IDS = 1
        };

        uint32_t subtypeMagic;
        uint16_t subtypeVersionHigh;
        uint16_t subtypeVersionLow;
        uint32_t flags;
        int32_t tileIndexPtr;
        int32_t stringTablePtr;
        int32_t indexSchemaPtr;
        clarisma::UUID guid;
        uint32_t revision;
        uint32_t modifiedSinceRevision;
        DateTime revisionTimestamp;
        DateTime modifiedSinceTimestamp;
        uint32_t sourceReplicationNumber;
        uint32_t tileCount;
        Settings settings;
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

#ifdef GEODESK_MULTITHREADED
    void addref()
    {
        refcount_.fetch_add(1, std::memory_order_relaxed);
    }

    void release()
    {
        if (refcount_.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            delete this;
        }
    }
#else
    void addref()  { ++refcount_;  }
    void release() { if (--refcount_ == 0) delete this;  }
    size_t refcount() const { return refcount_; }
#endif

    DataPtr tileIndex() const noexcept
    { 
        return { mainMapping() + header()->tileIndexPtr };
    }

    const clarisma::UUID& guid() const { return header()->guid; }
    uint32_t revision() const { return header()->revision; }
    DateTime revisionTimestamp() const { return header()->revisionTimestamp; }
    uint32_t tileCount() const { return header()->tileCount; }
    bool hasWaynodeIds() const { return header()->flags & Header::Flags::WAYNODE_IDS; }
    ZoomLevels zoomLevels() const { return zoomLevels_; }
    StringTable& strings() { return strings_; }
    const IndexedKeyMap& keysToCategories() const { return keysToCategories_; }
    std::vector<std::string_view> indexedKeyStrings() const;
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

    struct Metadata
    {
        Metadata(const clarisma::UUID& guid_) :
            guid(guid_), flags(0), revision(0),
            settings(nullptr),
            tileIndex(nullptr),
            stringTable(nullptr), stringTableSize(0),
            properties(nullptr), propertiesSize(0),
            indexedKeys(nullptr)
        {
        }

        clarisma::UUID guid;
        int flags;
        uint32_t revision;
        DateTime revisionTimestamp;
        const Settings* settings;
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
            tileIndexOfs_ = header->tileIndexPtr;
        }

        FeatureStore* store() const
        {
            return reinterpret_cast<FeatureStore*>(store_);
        }

        // TODO: We should cache this so we don't have to keep looking it up
        Header* getHeaderBlock()
        {
            return reinterpret_cast<Header*>(getRootBlock());
        }

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

    void readIndexSchema(DataPtr pSchema);

    void readTileSchema();

    static std::unordered_map<std::string, FeatureStore*>& getOpenStores();
    static std::mutex& getOpenStoresMutex();

#ifdef GEODESK_MULTITHREADED
    std::atomic_size_t refcount_;
#else
    std::size_t refcount_;
#endif

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