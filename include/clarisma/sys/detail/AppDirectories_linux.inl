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
    (void)appId;

    if (app.empty())
    {
        throw std::invalid_argument(
            "Application name must not be empty");
    }

    const char* var;
    const char* fallback;

    switch (kind)
    {
    case Kind::CACHE:
        var = "XDG_CACHE_HOME";
        fallback = ".cache";
        break;

    case Kind::CONFIG:
        var = "XDG_CONFIG_HOME";
        fallback = ".config";
        break;

    case Kind::DATA:
        var = "XDG_DATA_HOME";
        fallback = ".local/share";
        break;

    case Kind::STATE:
        var = "XDG_STATE_HOME";
        fallback = ".local/state";
        break;

    default:
        throw std::invalid_argument(
            "Invalid application-directory kind");
    }

    std::filesystem::path result;

    const char* value = std::getenv(var);
    if (value != nullptr && value[0] != '\0')
    {
        result = std::filesystem::path(value);
    }
    else
    {
        const char* home = std::getenv("HOME");
        if (home == nullptr || home[0] == '\0') return {};
        result = std::filesystem::path(home);
        result /= fallback;
    }
    if (!result.is_absolute()) return {};

    return result / app;
}

} // namespace clarisma::AppDirectories