// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <string_view>
#include <vector>
#include <clarisma/data/HashMap.h>

namespace geodesk {

class StringTable;

class KeySchema 
{
public:
    KeySchema(StringTable* strings, std::string_view keys);

    enum SpecialKey
    {
        ID, LON, LAT, TAGS
    };

    size_t columnCount() const { return columns_.size(); }
    int columnOfLocal(std::string_view key) const;
    int columnOfGlobal(int key) const;
    int columnOfSpecial(SpecialKey special) const;
    const std::vector<std::string_view>& columns() const { return columns_; }

    static constexpr int WILDCARD = INT_MAX;

private:
    void addKey(std::string_view key);
    int checkWildcard(std::string_view key) const;

    StringTable* strings_;
    std::vector<std::string_view> columns_;
    clarisma::HashMap<uint16_t,uint16_t> globals_;
    clarisma::HashMap<std::string_view,uint16_t> locals_;
    std::vector<std::string_view> startsWith_;
    std::vector<std::string_view> endsWith_;
};

} // namespace geodesk
