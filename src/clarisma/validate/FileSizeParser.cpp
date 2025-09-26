// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/validate/FileSizeParser.h>
#include <cmath>

namespace clarisma {

uint64_t FileSizeParser::parse()
{
    skipWhitespace();
    double num = number();
    if (std::isnan(num)) error("Expected number");
    skipWhitespace();
    char ch = *pNext_;
    if (ch != 0)
    ch = ch < 'a' ? ch : (ch - 32);     // turn lowercase
    if (ch == 0)
    {
        // do nothing
    }
    else if (ch == 'K')
    {
        num *= 1024.0;
    }
    else if (ch == 'M')
    {
        num *= 1024.0 * 1024;
    }
    else if (ch == 'G')
    {
        num *= 1024.0 * 1024 * 1024;
    }
    else if (ch == 'T')
    {
        num *= 1024.0 * 1024 * 1024 * 1024;
    }
    else if (ch == 'P')
    {
        num *= 1024.0 * 1024 * 1024 * 1024 * 1024;
    }
    else if (ch == 'E')
    {
        num *= 1024.0 * 1024 * 1024 * 1024 * 1024 * 1024;
    }
    else
    {
        error("Invalid unit");
    }
    return static_cast<uint64_t>(num);
}

} // namespace clarisma
