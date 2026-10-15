// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/format/GolStringSet.h>
#include <clarisma/util/Strings.h>
#include <geodesk/feature/StringTable.h>

namespace geodesk {

using namespace clarisma;

void GolStringSet::addStringList(std::string_view list, const StringTable* strings)
{
    size_t start = 0;
    for (;;)
    {
        size_t end = list.find(',', start);
        if (end == std::string_view::npos)
        {
            addString(list.substr(start), strings);
            break;
        }
        addString(list.substr(start, end - start), strings);
        start = end + 1;
    }
}

void GolStringSet::addString(std::string_view s, const StringTable* strings)
{
    s = Strings::trim(s);
    if (s.empty()) return;

    int code = strings->getCode(s);
    if (code >= 0)
    {
        globals_.emplace(static_cast<uint16_t>(code));
    }
    else
    {
        locals_.emplace(s);
    }
}

} // namespace geodesk

