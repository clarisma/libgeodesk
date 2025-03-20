// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/TagValue.h>
#include <clarisma/text/TextMetrics.h>

namespace geodesk {

bool TagValue::operator==(const TagValue& other) const noexcept
{
      // TODO
    return false;
}

int TagValue::charCount() const noexcept
{
    switch (type())
    {
    case 1:     // global string
    case 3:     // local string (fall through)
        return static_cast<int>(
            clarisma::TextMetrics::countCharsUtf8(stringValue_));
    case 0:     // narrow number
    {
        char buf[32];
        char* end = clarisma::Format::integer(buf,
            TagValues::intFromNarrowNumber(rawNumberValue()));
        return static_cast<int>(end - buf);
    }
    case 2:     // wide number
    {
        char buf[32];
        char* end = TagValues::decimalFromWideNumber(
            rawNumberValue()).format(buf);
        return static_cast<int>(end - buf);
    }
    default:
        UNREACHABLE_CASE
    }
}

} // namespace geodesk