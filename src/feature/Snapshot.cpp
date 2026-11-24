// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/Snapshot.h>

namespace geodesk {

using namespace clarisma;

void Snapshot::readIndexSchema(DataPtr p)
{
    int32_t count = p.getInt();
    keysToCategories_.reserve(count);
    for (int i = 0; i < count; i++)
    {
        p += 4;
        keysToCategories_.insert({ p.getUnsignedShort(), (p+2).getUnsignedShort() });
    }
}

int Snapshot::getIndexCategory(int keyCode) const
{
    auto it = keysToCategories_.find(keyCode);
    if (it != keysToCategories_.end())
    {
        return it->second;
    }
    return 0;
}


std::vector<std::string_view> Snapshot::indexedKeyStrings() const
{
    std::vector<std::string_view> keys;
    keys.reserve(keysToCategories_.size());
    for(auto& entry: keysToCategories_)
    {
        keys.emplace_back(strings_.getGlobalString(entry.first)->toStringView());
    }
    return keys;
}


} // namespace geodesk

