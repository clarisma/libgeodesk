// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/util/Json.h>
#include <clarisma/util/Buffer.h>


namespace clarisma {


void Json::writeEscaped(Buffer& out, std::string_view s)
{
    const char* p = s.data();
    const char* end = p + s.size();

    static constexpr char ESCAPE_TABLE[32][8] =
    {
        "\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005",
        "\\u0006", "\\u0007", "\\b",     "\\t",     "\\n",     "\\u000B",
        "\\f",     "\\r",     "\\u000E", "\\u000F", "\\u0010", "\\u0011",
        "\\u0012", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017",
        "\\u0018", "\\u0019", "\\u001A", "\\u001B", "\\u001C", "\\u001D",
        "\\u001E", "\\u001F"
    };

    while (p < end)
    {
        const char* start = p;

        // Fast-path: write raw bytes until an escape is needed
        while (p < end)
        {
            unsigned char ch = static_cast<unsigned char>(*p);
            if (ch == '\"' || ch == '\\' || ch < 0x20)
            {
                break;
            }
            ++p;
        }

        if (p > start) out.write(start, p - start);
        if (p == end) break;

        unsigned char ch = static_cast<unsigned char>(*p++);
        size_t len = 2;
        const char* esc;
        if (ch == '\"')
        {
            esc = "\\\"";
        }
        else if (ch == '\\')
        {
            esc = "\\\\";
        }
        else // ch < 0x20
        {
            esc = ESCAPE_TABLE[ch];
            len = (esc[1] == 'u') ? 6 : 2;
        }
        out.write(esc, len);
    }
}

} // namespace clarisma
