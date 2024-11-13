// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef _WIN32

#include <clarisma/sys/SystemInfo.h>
#include <windows.h>
#include <clarisma/cli/Console.h>

namespace clarisma {

SystemInfo::SystemInfo()
{
    HANDLE hProcess = GetCurrentProcess();
    if (!SetProcessWorkingSetSize(hProcess, 
        1ULL * 1024 * 1024 * 1024, 
        30ULL * 1024 * 1024 * 1024))
    {
        Console::msg("SetProcessWorkingSetSize() failed.");
        return;
    }
    if (!GetProcessWorkingSetSize(hProcess, &minWorkingSet, &maxWorkingSet))
    {
        Console::msg("GetProcessWorkingSetSize() failed.");
        return;
    }
    Console::msg("Min working set: %llu", minWorkingSet);
    Console::msg("Max working set: %llu", maxWorkingSet);
}

} // namespace clarisma

#endif