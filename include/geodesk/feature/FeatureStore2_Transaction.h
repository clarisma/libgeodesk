// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#ifdef GEODESK_LIBERO


#include <geodesk/feature/FeatureStore2.h>
#include <clarisma/libero/FreeStore_Transaction.h>

namespace geodesk {

struct FeatureStore::Metadata
{
    Metadata(const clarisma::UUID& guid_) :
        guid(guid_)
    {
    }

    clarisma::UUID guid;
    int flags = 0;
    uint32_t revision = 0;
    DateTime revisionTimestamp;
    const Settings* settings = nullptr;
    uint32_t tipCount = 0;
    const uint8_t* stringTable = nullptr;
    size_t stringTableSize = 0;
    const uint8_t* properties  = nullptr;
    size_t propertiesSize = 0;
    const uint32_t* indexedKeys = nullptr;
};

class FeatureStore::Transaction : public FreeStore::Transaction
{
public:
    explicit Transaction(FeatureStore& store) :
        FreeStore::Transaction(store) {}

    FeatureStore& store() const noexcept
    {
        return *reinterpret_cast<FeatureStore*>(&FreeStore::Transaction::store());
    }

    Header& header()
    {
        return *reinterpret_cast<Header*>(&FreeStore::Transaction::header());
    }

    void setup(const Metadata& metadata);
    // void addTile(Tip tip, std::span<byte> data);

protected:
    // uint32_t tileIndexOfs_;
};

} // namespace geodesk

// \endcond

#endif