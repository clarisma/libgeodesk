// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/io/FileTime.h>
#include <clarisma/io/File.h>

namespace clarisma {

#ifdef _WIN32
#include <windows.h>

static DateTime fromWindowsTime(FILETIME ft)
{
	uint64_t winTime = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
	constexpr uint64_t EPOCH_DIFFERENCE = 11644473600ULL; // in seconds
	return DateTime(winTime / 10000ULL
		- EPOCH_DIFFERENCE * 1000ULL);
}

FileTime::FileTime(const char* filename)
{
	File file;
    file.open(filename, File::OpenMode::READ);
	FILETIME created, accessed, modified;
	if (!GetFileTime(file.native(), &created, &accessed, &modified))
	{
		throw IOException();
	}
	created_ = fromWindowsTime(created);
	accessed_ = fromWindowsTime(accessed);
	modified_ = fromWindowsTime(modified);
}

#else

#include <sys/stat.h>
#include <ctime>

FileTime::FileTime(const char* filename)
{
	struct stat file_stat;
	if (stat(filename, &file_stat) == -1)
	{
		IOException::checkAndThrow();
	}

	modified_ = DateTime(static_cast<int64_t>(file_stat.st_mtime));
	accessed_ = DateTime(static_cast<int64_t>(file_stat.st_atime));

#ifdef __APPLE__
	created_ = DateTime(static_cast<int64_t>(file_stat.st_birthtime));
#else
	created_ = DateTime(file_stat.st_ctime);
#endif
}

#endif

} // namespace clarisma
