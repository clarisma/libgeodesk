// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <cstdlib>
#include <stdexcept>

namespace clarisma::AppDirectories {

inline std::filesystem::path tryPath(
    std::string_view vendor,
    std::string_view app,
    std::string_view appId,
    Kind kind)
{
    (void)vendor;

    if (app.empty())
    {
        throw std::invalid_argument(
            "Application name must not be empty");
    }

    const char* home = std::getenv("HOME");
    if (home == nullptr || home[0] == '\0') return {};

    std::filesystem::path result(home);
    if (!result.is_absolute()) return {};

    std::string_view subdir = appId.empty() ? app : appId;

    switch (kind)
    {
    case Kind::CACHE:
        result /= "Library/Caches";
        result /= subdir;
        break;

    case Kind::CONFIG:
        result /= "Library/Application Support";
        result /= subdir;
        result /= "config";
        break;

    case Kind::DATA:
        result /= "Library/Application Support";
        result /= subdir;
        result /= "data";
        break;

    case Kind::STATE:
        result /= "Library/Application Support";
        result /= subdir;
        result /= "state";
        break;

    default:
        throw std::invalid_argument(
            "Invalid application-directory kind");
    }
    return result;
}

} // namespace clarisma::AppDirectories