// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/format/KeySchema.h>
#include <clarisma/util/Strings.h>
#include <geodesk/feature/StringTable.h>

using namespace clarisma;

namespace geodesk {

KeySchema::KeySchema(StringTable* strings, std::string_view keys) :
    strings_(strings)
{
    size_t start = 0;
    for (;;)
    {
        size_t end = keys.find(',', start);
        if (end == std::string_view::npos)
        {
            addKey(keys.substr(start));
            break;
        }
        addKey(keys.substr(start, end - start));
        start = end + 1;
    }
}

void KeySchema::addKey(std::string_view key)
{
    key = Strings::trim(key);
    if (key.empty()) return;

    // TODO: "special" keys: lon, lat, etc.

    if (key[key.size()-1] == '*') [[unlikely]]
    {
        startsWith_.emplace_back(key.data(), key.size() - 1);
        return;
    }
    if (key[0] == '*') [[unlikely]]
    {
        endsWith_.emplace_back(key.data()+1, key.size() - 1);
        return;
    }

    columns_.push_back(key);
    int code = strings_->getCode(key);
    if (code >= 0 && code <= FeatureConstants::MAX_COMMON_KEY)
    {
        globals_[code] = columns_.size();
    }
    else
    {
        locals_[key] = columns_.size();
    }
}

int KeySchema::checkWildcard(std::string_view key) const
{
    for (auto s : startsWith_)
    {
        if (key.starts_with(s)) return WILDCARD;
    }
    for (auto s : endsWith_)
    {
        if (key.ends_with(s)) return WILDCARD;
    }
    return 0;
}

int KeySchema::columnOfLocal(std::string_view key) const
{
    auto it = locals_.find(key);
    if (it != locals_.end()) return it->second;
    return checkWildcard(key);
}

int KeySchema::columnOfGlobal(int key) const
{
    auto it = globals_.find(key);
    if (it != globals_.end()) return it->second;
    return checkWildcard(strings_->getGlobalString(key)->toStringView());
}

} // namespace geodesk
