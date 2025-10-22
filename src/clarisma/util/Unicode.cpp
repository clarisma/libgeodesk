// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/util/Unicode.h>
#ifdef _WIN32
#include <windows.h>
#endif
namespace clarisma {

#ifdef _WIN32
std::wstring Unicode::toWideString(std::string_view s)
{
    int size = static_cast<int>(s.size());
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.data(), size, NULL, 0);
    std::wstring wideString(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), size, &wideString[0], sizeNeeded);
    return wideString;
}
#endif

} // namespace clarisma
