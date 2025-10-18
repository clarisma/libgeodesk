// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
// Prevent Windows headers from clobbering min/max
#define NOMINMAX
#endif
#include <windows.h>
#include <clarisma/io/IOException.h>

namespace clarisma {


inline void FileSystem::makeWorkDir(const std::filesystem::path& path)
{
    std::filesystem::create_directories(path);
}

} // namespace clarisma
