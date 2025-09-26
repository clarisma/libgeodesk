// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/util/Strings.h>

namespace clarisma {

class HashedStringView : public std::string_view
{
public:
    template <typename S>
    HashedStringView(std::string_view s) :
        std::string_view(s.data(), s.length()),
        hash_(Strings::hash(s))
    {
    }

private:
    size_t hash_;
};

} // namespace clarisma



