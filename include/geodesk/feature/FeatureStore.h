// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <span>
#include <unordered_map>
#ifdef GEODESK_PYTHON
#include <Python.h>
#endif
#include <clarisma/libero/FreeStore.h>
#include <clarisma/thread/ThreadPool.h>
#include <clarisma/util/DateTime.h>
#include <clarisma/util/UUID.h>
#include <geodesk/export.h>
#include <geodesk/feature/Key.h>
#include <geodesk/feature/StringTable.h>
#include <geodesk/feature/TilePtr.h>
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
class GEODESK_API FeatureStore final : public clarisma::FreeStore
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

    struct Snapshot
    {
        uint32_t revision;
        uint32_t modifiedSinceRevision;
        DateTime revisionTimestamp;
        DateTime modifiedSinceTimestamp;
        uint32_t tileIndex;
        uint32_t tileCount;
        uint32_t reserved[8];
    };

    struct Header : FreeStore::Header
    {
        enum Flags
        {
            WAYNODE_IDS = 1
        };

        clarisma::UUID guid;
        uint32_t flags;
        int32_t stringTablePtr;
        int32_t indexSchemaPtr;
        int32_t propertiesPtr;
        Settings settings;
        uint32_t tipCount;
        uint32_t metadataChecksum;
        uint32_t tileIndexChecksum;
        uint32_t reserved[2];
        Snapshot snapshots[2];
        uint8_t urlLength;
        char url[245];
        uint8_t unused[2];
    };

    static_assert(sizeof(Header) == HEADER_SIZE - 8);

    FeatureStore();
    ~FeatureStore() override;

    static FeatureStore* openSingle(std::string_view fileName);

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

    const clarisma::UUID& guid() const { return header()->guid; }
    const Snapshot& snapshot() const { return header()->snapshots[header()->activeSnapshot]; }
    uint32_t revision() const { return snapshot().revision; }
    DateTime revisionTimestamp() const { return snapshot().revisionTimestamp; }
    uint32_t tileCount() const { return snapshot().tileCount; }

    bool hasWaynodeIds() const { return header()->flags & Header::Flags::WAYNODE_IDS; }
    ZoomLevels zoomLevels() const { return zoomLevels_; }
    StringTable& strings() { return strings_; }
    const IndexedKeyMap& keysToCategories() const { return keysToCategories_; }
    std::vector<std::string_view> indexedKeyStrings() const;
    int getIndexCategory(int keyCode) const;
    const Header* header() const
    {
        return reinterpret_cast<const Header*>(data());
    }
    std::span<byte> stringTableData() const;
    std::span<byte> propertiesData() const;

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

    // const uint32_t* tileIndex() const noexcept { return tileIndex_; }
    DataPtr tileIndex() const noexcept { return DataPtr(reinterpret_cast<byte*>(tileIndex_)); }
        // TODO: standardize on const uint32_t*?
    int tipCount() const noexcept { return header()->tipCount; }

    #ifdef GEODESK_PYTHON
    PyObject* getEmptyTags();
    PyFeatures* getEmptyFeatures();
    #endif

    clarisma::ThreadPool<TileQueryTask>& executor() { return executor_; }

    TilePtr fetchTile(Tip tip) const;
    static bool isTileValid(const byte* p);

    struct Metadata;
    class Transaction;

protected:
    void initialize(const byte* data) override;
    void gatherUsedRanges(std::vector<uint64_t>& ranges) override;

    DataPtr getPointer(int ofs) const
    {
        return DataPtr(data() + ofs).follow();
    }

private:
	static constexpr uint32_t MAGIC = 0x1CE50D6E;
    static constexpr uint32_t MAGIC_GOB = 0xE0F6B060;	//	(60 B0 F6 E0) "gob of geo"
    static constexpr uint16_t VERSION_HIGH = 2;
    static constexpr uint16_t VERSION_LOW = 0;

    void readIndexSchema(DataPtr pSchema);

    static std::unordered_map<std::string, FeatureStore*>& getOpenStores();
    static std::mutex& getOpenStoresMutex();

#ifdef GEODESK_MULTITHREADED
    std::atomic_size_t refcount_;
#else
    std::size_t refcount_;
#endif

    StringTable strings_;
    IndexedKeyMap keysToCategories_;
    uint32_t* tileIndex_ = nullptr;
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

