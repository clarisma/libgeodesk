// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/sys/SystemInfo.h>

#if defined(_WIN32)
#include <windows.h>

namespace clarisma {

size_t SystemInfo::maxMemory()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo))
    {
        return memInfo.ullAvailPhys;
    }
    return 0;
}

} // namespace clarisma

#elif defined(__linux__)
#include <sys/sysinfo.h>

namespace clarisma {

size_t SystemInfo::maxMemory()
{
    struct sysinfo memInfo;
    if (sysinfo(&memInfo) == 0)
    {
        return memInfo.freeram * memInfo.mem_unit;
    }
    return 0;
}

} // namespace clarisma

#elif defined(__APPLE__)
#include <sys/sysctl.h>

namespace clarisma {

size_t SystemInfo::maxMemory()
{
    int64_t memAvailable;
    size_t size = sizeof(memAvailable);
    if (sysctlbyname("hw.memsize", &memAvailable, &size, NULL, 0) == 0)
    {
        return static_cast<size_t>(memAvailable);
    }
    return 0;
}

} // namespace clarisma

#else

// Unknown platform
// TODO

namespace clarisma {

size_t SystemInfo::maxMemory()
{
    return 0;
}

} // namespace clarisma

#endif

