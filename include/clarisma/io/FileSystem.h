// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <filesystem>

namespace clarisma {

class FileSystem
{
public:
	static size_t getBlockSize(const char* path);
	static size_t getAvailableDiskSpace(const char* path);
	static void makeWorkDir(const std::filesystem::path& path);
};

} // namespace clarisma

#if defined(_WIN32)
#include "detail/FileSystem_win.inl"
#else
#include "detail/FileSystem_posix.inl"
#endif