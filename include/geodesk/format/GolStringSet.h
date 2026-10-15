// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdint>
#include <clarisma/data/HashSet.h>
#include <geodesk/feature/TagTablePtr.h>

// \cond lowlevel

namespace geodesk {

class StringTable;

/// A set of strings (local or global)
///
class GolStringSet
{
public:
    /// Adds strings from a comma-separated list
    ///
    /// @param list     the strings (must remain valid for the lifetime
    ///                 of the GolStringSet)
    /// @param strings  the string table (for global-code resolution)
    ///
    void addStringList(std::string_view list, const StringTable* strings);


    /// Adds a string
    ///
    /// @param s        the string (must remain valid for the lifetime
    ///                 of the GolStringSet)
    /// @param strings  the string table (for global-code resolution)
    ///
    void addString(std::string_view s, const StringTable* strings);

    bool hasCode(int code) const
    {
        return globals_.contains(static_cast<uint16_t>(code));
    }
    bool hasString(std::string_view string) const
    {
        return locals_.contains(string);
    }

private:
    clarisma::HashSet<uint16_t> globals_;
    clarisma::HashSet<std::string_view> locals_;
};

} // namespace geodesk

/// \endcond