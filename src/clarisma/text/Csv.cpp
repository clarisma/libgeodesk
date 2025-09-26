// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/text/Csv.h>
#include <clarisma/util/Buffer.h>


namespace clarisma {

void Csv::writeEscaped(Buffer& out, std::string_view s)
{
    bool mustQuote = false;
    for (char ch : s)
    {
        if (ch == '"' || ch == ',' || ch == '\r' || ch == '\n')
        {
            mustQuote = true;
            break;
        }
    }

    if (!mustQuote)
    {
        out.write(s.data(), s.size());
        return;
    }

    out.write("\"", 1);
    const char* p = s.data();
    const char* end = p + s.size();

    while (p < end)
    {
        const char* start = p;

        // Write until the next quote
        while (p < end && *p != '"')
        {
            ++p;
        }

        out.write(start, p - start);

        // Escape quotes by doubling them
        if (p < end && *p == '"')
        {
            out.write("\"\"", 2);
            ++p;
        }
    }

    out.write("\"", 1);
}

} // namespace clarisma
