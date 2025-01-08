// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <string_view>

namespace clarisma {

class PropertiesParser 
{
public:
    PropertiesParser(std::string_view properties) :
        p_(properties.data()), end_(properties.data() + properties.size()) {}

    bool next(std::string_view &key, std::string_view &value);

private:
    const char* p_;
    const char* end_;
};

} // namespace clarisma