// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <streambuf>
#include <string_view>

namespace clarisma {

class StringViewBuffer : public std::streambuf
{
public:
    StringViewBuffer(std::string_view sv)
    {
        char *p = const_cast<char *>(sv.data());
        // Set the get area of the buffer: [p, p + size)
        setg(p, p, p + sv.size());
    }
};

} // namespace clarisma