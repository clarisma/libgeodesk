// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/io/IOException.h>
#include <clarisma/io/FileHandle.h>

#if defined(_WIN32) 
#include <Windows.h>
#endif

namespace clarisma {

IOException::IOException()
    : std::runtime_error(FileHandle::errorMessage())
{
}

#if defined(_WIN32)
std::string IOException::getMessage(const char* moduleName, int errorCode)
{
    HMODULE hModule = LoadLibraryA(moduleName);
    LPSTR buffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_IGNORE_INSERTS, hModule, errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buffer, 0, NULL);
    std::string message(buffer, size);
    LocalFree(buffer);
    return message;
}
#endif


} // namespace clarisma
