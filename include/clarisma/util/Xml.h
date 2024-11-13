// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstdlib>
#include <cstring>
#include <string_view>
#include <clarisma/util/Unicode.h>

namespace clarisma {

class Xml
{
public:
  	/// @brief Replaces XML entities in-place.
  	/// Invalid entities are silently removed.
    ///
    /// @return pointer to the terminating null char
  	static char* unescapeInplace(char* s)
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
};

} // namespace clarisma
