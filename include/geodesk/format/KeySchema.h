// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdint>
#include <vector>
#include <clarisma/data/HashMap.h>
#include <geodesk/feature/TagTablePtr.h>

// \cond lowlevel

namespace geodesk {

class StringTable;

class KeySchema 
{
public:
    KeySchema(StringTable* strings) : strings_(strings) {}
    KeySchema(StringTable* strings, std::string_view keys);

    enum SpecialKey
    {
        ID, LON, LAT, TAGS
    };

    static constexpr int SPECIAL_KEY_COUNT = 4;

    void addKeys(std::string_view keys);
    size_t columnCount() const { return columns_.size(); }
    int columnOfLocal(std::string_view key) const;
    int columnOfGlobal(int key) const;
    int columnOfSpecial(SpecialKey special) const
    {
        return specialKeyCols_[special];
    };
    const std::vector<std::string_view>& columns() const { return columns_; }

    static constexpr int WILDCARD = -1;

private:
    void addKey(std::string_view key);
    int checkWildcard(std::string_view key) const;

    StringTable* strings_;
    std::vector<std::string_view> columns_;
    clarisma::HashMap<uint16_t,uint16_t> globals_;
    clarisma::HashMap<std::string_view,uint16_t> locals_;
    std::vector<std::string_view> startsWith_;
    std::vector<std::string_view> endsWith_;
    uint16_t specialKeyCols_[SPECIAL_KEY_COUNT] = {};
};

} // namespace geodesk

// \endcond