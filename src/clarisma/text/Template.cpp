// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/text/Template.h>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace clarisma {

std::unique_ptr<Template> Template::compile(std::string_view text)
{
    struct Part
    {
        uint32_t ofs;
        uint32_t len;
    };
    std::vector<Part> parts;
    uint32_t totalTextLen = 0;

    parts.reserve(8);

    const char* start = text.data();
    const char* end = start + text.size();
    const char* p = start;
    const char* literalStart = p;
    uint32_t len;

    while (p < end)
    {
        if (*p == '{')
        {
            len = p-literalStart;
            totalTextLen += len;
            parts.emplace_back(literalStart-start, len);
            ++p;
            while (p < end && *p <= 32) // Skip whitespace
            {
                ++p;
            }
            const char* paramStart = p;
            bool foundClosingBrace = false;
            while (p < end)
            {
                if (*p == '}')
                {
                    foundClosingBrace = true;
                    break;
                }
                ++p;
            }
            if (!foundClosingBrace)
            {
                throw std::runtime_error("Unclosed '{' in template");
            }

            // Trim trailing whitespace before '}'.
            const char* paramEnd = p;
            while (paramEnd > paramStart && *paramEnd <= 32)
            {
                --paramEnd;
            }

            if (paramEnd <= paramStart)
            {
                throw std::runtime_error("Empty parameter name");
            }

            len = paramEnd-paramStart;
            totalTextLen += len;
            parts.emplace_back(paramStart-start, len);
            literalStart = p + 1;
        }
        ++p;
    }
    len = p-literalStart;
    totalTextLen += len;
    parts.emplace_back(p-start, len);

    size_t textOfs =
        sizeof(uint32_t) +
        parts.size() * sizeof(uint32_t);

    size_t allocSize = textOfs + totalTextLen;
    byte* bytes = new byte[allocSize];
    byte* pMeta = bytes;
    byte* pText = bytes + textOfs;
    *reinterpret_cast<uint32_t*>(pMeta) = textOfs;
    pMeta += sizeof(uint32_t);

    for (auto [ofs,len] : parts)
    {
        *reinterpret_cast<uint32_t*>(pMeta) = len;
        memcpy(pText, start+ofs, len);
        pMeta += sizeof(uint32_t);
        pText += len;
    }
    assert(pText == bytes + allocSize);
    return std::unique_ptr<Template>(reinterpret_cast<Template*>(bytes));
}

} // namespace clarisma
