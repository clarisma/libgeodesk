// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/util/PropertiesParser.h>
#include <clarisma/util/Strings.h>

namespace clarisma {

bool PropertiesParser::next(std::string_view &key, std::string_view &value)
{
    while(p_ < end_)
    {
        const char *pNext = p_;
        while(pNext < end_)
        {
            if(*pNext++ == '\n') break;
        }
        std::string_view line(p_, pNext);
        p_ = pNext;
        if(line.empty() || line[0] == '#') continue;
            // skip empty line or comment
        size_t pos = line.find('=');
        if(pos == std::string_view::npos) continue;    // skip line without =
        key = Strings::trim(line.substr(0, pos));
        value = Strings::trim(line.substr(pos+1));
        return true;
    }
    return false;
}

} // namespace clarisma