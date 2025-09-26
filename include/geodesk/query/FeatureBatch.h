// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <memory>
#include "QueryResults.h"

namespace geodesk {

class FeatureBatch
{
public:
    FeatureBatch(const QueryResults *results) : results_(results) {};
    ~FeatureBatch()
    {
        const QueryResults *p = results_;
        while(p)
        {
            const QueryResults *next = p->next;
            delete p;
            p = next;
        }
    }

    std::unique_ptr<const QueryResults> takeNext()
    {
        const QueryResults *p = results_;
        results_ = p->next;
        return std::unique_ptr<const QueryResults>(p);
    }

private:
    const QueryResults *results_;
};

} // namespace geodesk

