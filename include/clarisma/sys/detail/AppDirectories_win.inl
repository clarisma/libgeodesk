// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <stdexcept>
#include <string>
#include <system_error>

#ifndef NOMINMAX
#define NOMINMAX
#define CLARISMA_APP_DIRECTORIES_UNDEFINE_NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define CLARISMA_APP_DIRECTORIES_UNDEFINE_LEAN_AND_MEAN
#endif

#include <windows.h>

#ifdef CLARISMA_APP_DIRECTORIES_UNDEFINE_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#undef CLARISMA_APP_DIRECTORIES_UNDEFINE_LEAN_AND_MEAN
#endif

#ifdef CLARISMA_APP_DIRECTORIES_UNDEFINE_NOMINMAX
#undef NOMINMAX
#undef CLARISMA_APP_DIRECTORIES_UNDEFINE_NOMINMAX
#endif

namespace clarisma::AppDirectories {

inline std::filesystem::path tryPath(
    std::string_view vendor,
    std::string_view app,
    std::string_view appId,
    Kind kind)
{
    (void)appId;

    if (app.empty())
    {
        throw std::invalid_argument(
            "Application name must not be empty");
    }

    const wchar_t* variable;
    const wchar_t* category;

    switch (kind)
    {
    case Kind::CACHE:
        variable = L"LOCALAPPDATA";
        category = L"cache";
        break;

    case Kind::CONFIG:
        variable = L"APPDATA";
        break;

    case Kind::DATA:
        variable = L"LOCALAPPDATA";
        category = L"data";
        break;

    case Kind::STATE:
        variable = L"LOCALAPPDATA";
        category = L"state";
        break;

    default:
        throw std::invalid_argument(
            "Invalid application-directory kind");
    }

    wchar_t buf[1024];
    DWORD len = GetEnvironmentVariableW(variable, buf,
        static_cast<DWORD>(std::size(buf)));
    if (len == 0 || len >= std::size(buf)) return {};

    std::filesystem::path result(buf);
    if (!result.is_absolute()) return {};

    if (!vendor.empty()) result /= vendor;
    result /= app;

    result /= category;

    return result;
}

} // namespace clarisma::AppDirectories