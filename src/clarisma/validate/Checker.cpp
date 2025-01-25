// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/validate/Checker.h>

namespace clarisma {

template <typename... Args>
void Checker::error(uint64_t location, Error::Severity severity,
        const char* msg, Args... args)
{
    errors_.emplace_back(location, severity, Format::format(msg, args...));
}

} // namespace clarisma