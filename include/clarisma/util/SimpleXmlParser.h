// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/Parser.h>

namespace clarisma {

class SimpleXmlParser 
{
public:
    enum
    {
        END,
        TAG_START,
        ATTR,
        TEXT
    };

    enum class Error
    {
        NONE,
        EXPECTED_EQUAL,
        EXPECTED_QUOTE,
        INCOMPLETE_TAG,
        INCOMPLETE_DECLARATION,
        INCOMPLETE_COMMENT
    };


    explicit SimpleXmlParser(char *src) :
        p_(src),
        error_(Error::NONE),
        insideTag_(false),
        atTagStart_(false)
    {
    }

    int next();

    std::string_view name() const noexcept { return name_; };
    std::string_view value() const noexcept { return value_; };
    Error error() const noexcept { return error_; };

    int64_t longValue() const
    {
        return strtoll(value().data(), nullptr, 10);
    }

    double doubleValue() const
    {
        return strtod(value().data(), nullptr);
    }

private:
    void skipWhitespace()
    {
        for (;;)
        {
            unsigned char ch = static_cast<unsigned char>(*p_) - 1;
            if (ch > 31) break;
            p_++;
        }
    }

    bool parseName();
    void parseValue(char endChar, bool isAttribute);
    bool parseDeclaration();
    bool parseComment();

    static const CharSchema TAG_CHARS;

    char* p_;
    std::string_view name_;
    std::string_view value_;
    Error error_;
    /// Indicates that p_ is inside a start tag
    bool insideTag_;
    /// Indicates that p_ points to the character after <
    /// (The < char may have been replaced by a null-terminator)
    bool atTagStart_;
};

} // namespace clarisma
