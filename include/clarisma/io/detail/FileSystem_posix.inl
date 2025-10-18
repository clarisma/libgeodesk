// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <cstddef>
#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __linux__
#include <linux/fs.h>
#endif
#include <clarisma/io/IOException.h>

namespace clarisma {

inline void FileSystem::makeWorkDir(const std::filesystem::path& path)
{
    std::filesystem::create_directories(path);
#ifdef __linux__
    // Best effort: open the dir and set FS_NOCOW_FL. Ignore failures.
    int fd = ::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0)
    {
        return; // cannot tweak flags; directory still exists
    }

    int flags = 0;
    if (::ioctl(fd, FS_IOC_GETFLAGS, &flags) == 0)
    {
        int newFlags = flags | FS_NOCOW_FL;
        (void)::ioctl(fd, FS_IOC_SETFLAGS, &newFlags);
    }
    ::close(fd);
#endif
}

} // namespace clarisma
