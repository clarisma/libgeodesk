// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/data/HashMap.h>
#include <geodesk/feature/Key.h>
#include <geodesk/feature/StringTable.h>
#include <geodesk/feature/ZoomLevels.h>
#include <geodesk/match/Matcher.h>

// \cond

namespace geodesk {

/// A Snapshot represents a revision of a GOL.
/// TODO: Currently, this only includes fields used by the Matcher
///  architecture; once we move to v3, most of the fields from FeatureStore
///  will migrate here, and this will become a first-level refcounted object
///
/// \cond lowlevel
///
class Snapshot
{
public:
    using IndexedKeyMap = clarisma::HashMap<uint16_t, uint16_t>;
    using DataPtr = clarisma::DataPtr;

    const StringTable& strings() const { return strings_; }
    const IndexedKeyMap& keysToCategories() const { return keysToCategories_; }
    std::vector<std::string_view> indexedKeyStrings() const;
    int getIndexCategory(int keyCode) const;

    Key key(std::string_view k) const
    {
        int code = strings_.getCode(k.data(), k.size());
        return Key(k.data(), static_cast<uint32_t>(k.size()),
            code > TagValues::MAX_COMMON_KEY ? -1 : code);
    }

private:
    void initStrings(const uint8_t* pStrings)
    {
        strings_.create(pStrings);
    }

    void readIndexSchema(DataPtr pSchema);

    StringTable strings_;
    IndexedKeyMap keysToCategories_;

    friend class FeatureStore;
};

} // namespace geodesk

// \endcond

