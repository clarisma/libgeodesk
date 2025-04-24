// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only



#pragma once
#include <cstdint>
#include <vector>
#include <clarisma/data/HashMap.h>
#include <geodesk/feature/GlobalTagIterator.h>
#include <geodesk/feature/LocalTagIterator.h>
#include <geodesk/feature/TagTablePtr.h>

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

    static constexpr int WILDCARD = -1;

    template<typename Func>
    void forEach(TagTablePtr tags, Func&& func)
    {
        int32_t fakeHandle = static_cast<int32_t>(tags.ptr()) & 2;
        // We don't need the handle; however, we need to respect the alignment
        // to ensure that pointer calculations work; hence, handle will be 0 or 2

        GlobalTagIterator iterGlobal(fakeHandle, tags);
        while(iterGlobal.next())
        {
            if(iterGlobal.hasLocalStringValue())
            {
                addGlobalTag(iterGlobal.key(), iterGlobal.localStringValue()->toStringView());
            }
            else
            {
                addGlobalTag(iterGlobal.key(), iterGlobal.valueType(), iterGlobal.value());
            }
        }
        if(p.hasLocalKeys())
        {
            LocalTagIterator iterLocal(fakeHandle, p);
            while(iterLocal.next())
            {
                std::string_view key = iterLocal.keyString()->toStringView();
                if(iterLocal.hasLocalStringValue())
                {
                    addLocalTag(key, iterLocal.localStringValue()->toStringView());
                }
                else
                {
                    addLocalTag(key, iterLocal.valueType(), iterLocal.value());
                }
            }
        }
        GlobalTagIterator iterGlobal(fakeHandle, p);
        while(iterGlobal.next())
        {
            if(iterGlobal.hasLocalStringValue())
            {
                addGlobalTag(iterGlobal.key(), iterGlobal.localStringValue()->toStringView());
            }
            else
            {
                addGlobalTag(iterGlobal.key(), iterGlobal.valueType(), iterGlobal.value());
            }
        }
        // Example stub usage
        func(StringValue("key1"), TagValue("value1"));
        func(StringValue("key2"), TagValue(42));
    }

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

