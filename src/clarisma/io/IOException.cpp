// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/io/IOException.h>

#if defined(_WIN32) 
#include <Windows.h>

namespace clarisma {

/*
void IOException::getError(char* buf)
{

}
*/


std::string IOException::getMessage(int errorCode)
{
    LPSTR buffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buffer, 0, NULL);
    std::string message(buffer, size);
    LocalFree(buffer);
    return message;
}

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

void IOException::checkAndThrow()
{
	DWORD errorCode = GetLastError();
	if (errorCode == 0) return;
    throw IOException(static_cast<int>(errorCode));
}

#elif defined(__linux__) || defined(__APPLE__) 

#include <cerrno>
#include <cstring>

namespace clarisma {

void IOException::checkAndThrow()
{
    if (errno)
    {
        // LOG("errno = %d", errno);
        char buf[256];
        strcpy(buf, "Error while retrieving message");
        strerror_r(errno, buf, sizeof(buf));
        throw IOException(buf);
    }
}

#else
#error "Platform not supported"
#endif

void IOException::alwaysThrow()
{
    checkAndThrow();
    throw IOException("Unknown IO error");
}

} // namespace clarisma
