// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdint>
#include <string>
#include <geodesk/export.h>

namespace geodesk {

class Tags;
class Selection;

/// \cond internal
///
class GEODESK_API FeatureUtils
{
public:
    static char* format(char* buf, const char* type, int64_t id);
    static std::string label(const Tags& tags);
};

// \endcond

} // namespace geodesk