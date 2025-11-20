// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/util/SimpleXmlParser.h>
#include <clarisma/util/Xml.h>

namespace clarisma {

const CharSchema SimpleXmlParser::TAG_CHARS
{
    0b0000011111111111000000000000000000000000000000000000000000000000,
    0b0000011111111111111111111111111010000111111111111111111111111110,
    0b1111111111111111111111111111111111111111111111111111111111111111,
    0b1111111111111111111111111111111111111111111111111111111111111111,
};

int SimpleXmlParser::next()
{
    char ch = *p_;
    if(insideTag_)
    {
        skipWhitespace();
        if(parseName())
        {
            // Attribute
            skipWhitespace();
            if(*p_ != '=')
            {
                error_ = Error::EXPECTED_EQUAL;
                return END;
            }
            p_++;
            skipWhitespace();
            char quote = *p_;
            if(quote != '\'' && quote != '\"')
            {
                error_ = Error::EXPECTED_QUOTE;
                return END;
            }
            p_++;
            parseValue(quote, true);
            return ATTR;
        }
        insideTag_ = false;
        ch = *p_;
        if(ch == 0)
        {
            error_ = Error::INCOMPLETE_TAG;
            return END;
        }
        p_++;
        if(ch != '>')
        {
            if(ch == '/')
            {
                ch = *p_;
                if(ch != '>')
                {
                    error_ = Error::INCOMPLETE_TAG;
                    return END;
                }
                p_++;
            }
            else
            {
                error_ = Error::INCOMPLETE_TAG;
            }
            return END;
        }
        ch = *p_;
        // fall through
    }
    else if (atTagStart_)
    {
        ch = '<';
        p_--;
        atTagStart_ = false;
    }
    if(ch == 0) return END;
    if(ch == '<')
    {
        p_++;
        ch = *p_;
        if(ch == 0)
        {
            error_ = Error::INCOMPLETE_TAG;
            return END;
        }
        if(ch == '/')
        {
            p_++;
            if(!parseName())
            {
                error_ = Error::INCOMPLETE_TAG;
            }
            ch = *p_;
            if(ch == '>')
            {
                p_++;
            }
            else
            {
                error_ = Error::INCOMPLETE_TAG;
            }
            return END;
        }
        if(parseName())
        {
            insideTag_ = true;
            return TAG_START;
        }
        if(ch == '?')
        {
            if(!parseDeclaration())
            {
                error_ = Error::INCOMPLETE_DECLARATION;
                return END;
            }
            // fall through
        }
        else if(ch == '!')
        {
            if(!parseComment())
            {
                error_ = Error::INCOMPLETE_COMMENT;
                return END;
            }
            // fall through
        }
        else
        {
            error_ = Error::INCOMPLETE_TAG;
            return END;
        }
    }
    parseValue('<', false);
    return TEXT;
}


bool SimpleXmlParser::parseName()
{
    const char* start = p_;
    for(;;)
    {
        char ch = *p_;
        if(!TAG_CHARS.test(ch)) break;
        p_++;
    }
    size_t len = p_ - start;
    name_ = std::string_view(start, len);
    return len != 0;
}


void SimpleXmlParser::parseValue(char endChar, bool isAttribute)
{
    bool hasEntities = false;
    char* start = p_;
    char* end;
    for(;;)
    {
        char ch = *p_;
        if(ch == 0)
        {
            if(isAttribute) error_ = Error::EXPECTED_QUOTE;
            end = p_;
            break;
        }
        p_++;
        if(ch == endChar)
        {
            end = p_ - 1;
            atTagStart_ = !isAttribute;
            break;
        }
        hasEntities |= (ch == '&');
    }
    *end = '\0';
    if(hasEntities)
    {
        end = Xml::unescapeInplace(start);
    }
    value_ = std::string_view(start, end);
}


bool SimpleXmlParser::parseDeclaration()
{
    for(;;)
    {
        p_++;
        char ch = *p_;
        if(ch == 0) return false;
        if(ch == '?') break;
    }
    p_++;
    if(*p_ != '>') return false;
    p_++;
    return true;
}

/// @brief Skips an XML comment
/// Expects p_ to point to '!' in "<!..."
/// @returns true if valid comment (p_ points to char after closing '>'),
///    or false if comment is malformed
bool SimpleXmlParser::parseComment()
{
    p_++;
    if(*p_ != '-') return false;
    p_++;
    if(*p_ != '-') return false;
    for(;;)
    {
        p_++;
        char ch = *p_;
        if(ch == 0) return false;
        if(ch == '-')
        {
            if(*(p_ + 1) == '-')
            {
                p_ += 2;
                break;
            }
        }
    }
    if(*p_ != '>') return false;
    p_++;
    return true;
}

} // namespace clarisma
