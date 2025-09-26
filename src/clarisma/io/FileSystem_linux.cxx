// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/io/FileSystem.h>
#include <iostream>
#include <sys/statvfs.h>
#include <clarisma/io/IOException.h>

namespace clarisma {

size_t FileSystem::getBlockSize(const char* path)
{
    struct statvfs buf;
    if (statvfs(path, &buf) != 0) 
    {
        throw IOException();
        return 0;
    }
    return buf.f_bsize;
}


size_t FileSystem::getAvailableDiskSpace(const char* path)
{
    struct statvfs buf;
    if (statvfs(path, &buf) != 0)
    {
        throw IOException();
    }
    return static_cast<size_t>(buf.f_bsize) * buf.f_bavail;
}

} // namespace clarisma