// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/io/FileSystem.h>
#include <windows.h>
#include <clarisma/io/IOException.h>

namespace clarisma {

size_t FileSystem::getBlockSize(const char* path)
{
    DWORD sectorsPerCluster, bytesPerSector, numberOfFreeClusters, totalNumberOfClusters;

    if (!GetDiskFreeSpaceA(path, &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters)) 
    {
        throw IOException();
    }
    return sectorsPerCluster * bytesPerSector;
}


size_t FileSystem::getAvailableDiskSpace(const char* path)
{
    ULARGE_INTEGER freeBytesAvailable;
    if (!GetDiskFreeSpaceExA(path, &freeBytesAvailable, nullptr, nullptr)) 
    {
        throw IOException();
    }
    return freeBytesAvailable.QuadPart;
}

} // namespace clarisma