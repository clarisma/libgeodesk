// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/FeatureIteratorBase.h>

namespace geodesk {

/// \cond

template<typename T>
class FeatureIterator : protected FeatureIteratorBase
{
public:
    explicit FeatureIterator(const View& view) :
        FeatureIteratorBase(view) {}

    // Dereference operator
    T operator*() const noexcept
    {
        return reinterpret_cast<const T&>(currentFeature());
    }

    // Increment operator
    FeatureIterator& operator++()
    {
        fetchNext();
        return *this;
    }

    // Comparison operator with nullptr
    bool operator!=(std::nullptr_t) const
    {
        return !currentFeature().isNull();
    }

    bool operator==(std::nullptr_t) const
    {
        return currentFeature().isNull();
    }
};

// \endcond

} // namespace geodesk
