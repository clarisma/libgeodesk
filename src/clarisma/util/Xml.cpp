// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/util/Xml.h>
#include <clarisma/text/Format.h>
#include <clarisma/util/BufferWriter.h>


namespace clarisma {

char* Xml::unescapeInplace(char* s)
{
    char* write = s; // Pointer for writing unescaped characters
    char* read = s;  // Pointer for reading the original string

    while (*read)
    {
        if (*read == '&')
        {
            read++;
            char* entity = read;
            for(;;)
            {
                char ch = *read++;
                if(ch == 0)
                {
                    *write = '\0';
                    return write;
                }
                if(ch == ';') break;
            }
            *(read-1) = '\0';

            if(*entity != '#')
            {
                std::string_view e(entity, read-1);

                // Decode common entities
                if (e == "amp")
                {
                    *write++ = '&';
                }
                else if (e == "lt")
                {
                    *write++ = '<';
                }
                else if (e == "gt")
                {
                    *write++ = '>';
                }
                else if (e == "quot")
                {
                    *write++ = '"';
                }
                else if (e == "apos")
                {
                    *write++ = '\'';
                }
            }
            else
            {
                long code;
                if (entity[1] == 'x' || entity[1] == 'X')
                {
                    // Hexadecimal: &#x...;
                    code = strtol(entity+1, nullptr, 16);
                }
                else
                {
                    // Decimal: &#...;
                    code = strtol(entity,nullptr, 10);
                }
                // Convert codepoint to UTF-8 and write it in place
                if(code) Unicode::encode(write, static_cast<int>(code));
            }
        }
        else
        {
            // Copy non-entity characters
            *write++ = *read++;
        }
    }
    *write = '\0'; // Null-terminate the result
    return write;
}

void Xml::writeEscaped(Buffer& out, std::string_view s)
{
    const char* start = s.data();
    const char* end = start + s.size();
    const char* lastWritten = start;

    for (const char* p = start; p < end; ++p)
    {
        char currentChar = *p;
        const char* replacement = nullptr;
        size_t replacementLen = 0;
        char hexBuf[8];

        switch (currentChar)
        {
        case '&':
            replacement = "&amp;";
            replacementLen = 5;
            break;

        case '<':
            replacement = "&lt;";
            replacementLen = 4;
            break;

        case '>':
            replacement = "&gt;";
            replacementLen = 4;
            break;

        case '"':
            replacement = "&quot;";
            replacementLen = 6;
            break;

        case '\'':
            replacement = "&apos;";
            replacementLen = 6;
            break;

        default:
        {
            unsigned char uc = static_cast<unsigned char>(currentChar);
            if (uc < 32)
            {
                // Convert unprintable character to "&#xHH;"
                hexBuf[0] = '&';
                hexBuf[1] = '#';
                hexBuf[2] = 'x';
                Format::hex(hexBuf + 3, uc, 2); // 2 hex digits for one byte (00–ff)
                hexBuf[5] = ';';
                replacement = hexBuf;
                replacementLen = 6; // "&" + "#" + "x" + 2 hex digits + ";"
            }
            break;
        }
        }

        if (replacement)
        {
            // Write everything before the special/unprintable char
            if (p > lastWritten)
            {
                out.write(lastWritten, p - lastWritten);
            }

            // Write the replacement sequence
            out.write(replacement, replacementLen);

            // Move past the replaced character
            lastWritten = p + 1;
        }
    }

    // Write any remaining text after the loop
    if (lastWritten < end)
    {
        out.write(lastWritten, end - lastWritten);
    }
}

} // namespace clarisma
