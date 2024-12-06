// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstdint>
#include <clarisma/util/DataPtr.h>
#include <geodesk/feature/FeaturePtr.h>

namespace geodesk {

class QueryBase;
struct QueryResults;

/// \cond lowlevel

struct QueryResultsHeader
{
    QueryResults* next;
    clarisma::DataPtr pTile;
    uint32_t count;
};

struct QueryResults : public QueryResultsHeader
{
    static const uint32_t DEFAULT_BUCKET_SIZE = 256;

    static QueryResultsHeader EMPTY_HEADER;
    static QueryResults* const EMPTY;

    bool isFull() const
    {
        return count == DEFAULT_BUCKET_SIZE;
    }

    class Iterator
    {
    public:
        Iterator(const QueryResults* results, int n) :
            p_(&results->items[n]),
            pTile_(results->pTile)
        {
        }

        FeaturePtr operator*() const
        {
            return FeaturePtr(pTile_ + *p_);
        }

        Iterator& operator++()
        {
            ++p_;
            return *this;
        }

        bool operator==(const Iterator& other) const
        {
            return p_ == other.p_;
        }

        bool operator!=(const Iterator& other) const
        {
            return p_ != other.p_;
        }

    private:
        const uint32_t* p_;
        clarisma::DataPtr pTile_;
    };

    Iterator begin() const
    {
        return Iterator(this, 0);
    }

    Iterator end() const
    {
        return Iterator(this, count);
    }

    uint32_t items[DEFAULT_BUCKET_SIZE];
};

using QueryResultsConsumer = void(*)(QueryBase* query, QueryResults*);

// \endcond

} // namespace geodesk
