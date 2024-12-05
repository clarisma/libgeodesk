// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstring>
#include <string>
#include <string_view>

namespace clarisma {

class FilePath
{
public:
    /// @brief Returns the extension of the given filename (as pointer to ".ext"),
    /// or an empty string if the filename does not have an extension.
    ///
    /// TODO: Fix, assumes null-terminated string!
    static const char* extension(const char* path, size_t len);
    static const char* extension(const char* path)
    {
        return extension(path, strlen(path));
    }
    static const char* extension(std::string_view path)
    {
        return extension(path.data(), path.length());
    }

    static std::string_view withoutExtension(std::string_view path)
    {
        const char* currentExt = extension(path);
        if(*currentExt == 0) return path;
        return { path.data(), static_cast<size_t>(currentExt - path.data()) };
    }

    static std::string withExtension(std::string_view path, const char* ext)
    {
        return std::string(withoutExtension(path)) + ext;
    }

    static std::string withDefaultExtension(std::string_view path, const char* ext)
    {
        return (*extension(path) != 0) ? std::string(path) : std::string(path) + ext;
    }

    static std::string_view name(std::string_view path);
};

} // namespace clarisma
