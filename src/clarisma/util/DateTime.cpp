// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/util/DateTime.h>

namespace clarisma {

void DateTime::format(char* buf, size_t bufSize, const char* format) const
{
    time_t seconds = timestamp_ / 1000;
    std::tm tm;
#ifdef _WIN32
    // Use gmtime_s on Windows (returns an error code)
    gmtime_s(&tm, &seconds);
#else
    gmtime_r(&seconds, &tm);
#endif
    // Format date/time into the buffer
    std::strftime(buf, bufSize, format, &tm);
}

void DateTime::format(char* buf) const
{
    // 2024-10-30 12:51:00
    format(buf, 20, "%Y-%m-%d %H:%M:%S");
}

} // namespace clarisma
