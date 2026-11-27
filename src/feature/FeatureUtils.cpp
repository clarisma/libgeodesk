// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/FeatureUtils.h>
#include <clarisma/text/Format.h>
#include <clarisma/util/StringBuilder.h>
#include <geodesk/feature/FeatureIterator.h>
#include <geodesk/feature/Tags.h>
#include <geodesk/query/Selection.h>

using namespace clarisma;

namespace geodesk {

char* FeatureUtils::format(char* buf, const char* type, int64_t id)
{
    char* p = buf;
    while (*type) *p++ = *type++;
    *p = '/';
    return clarisma::Format::integer(p + 1, id);
}

std::string FeatureUtils::label(const Tags& tags)
{
    TagValue type;
    for(Tag tag : tags)
    {
        if(tag.value())
        {
            if(tag.key().startsWith("addr:"))
            {
                return "address";
            }
            type = tag.value();
            if(type == "yes")
            {
                type = tag.key();
            }
            break;
        }
    }
    TagValue name = tags["name"];

    StringBuilder str;
    if(type)
    {
        str << type;
    }
    else
    {
        str << "feature";
    }
    if(name)
    {
        str << " \"" << name << '\"';
    }
    return str.toString();
}

} // namespace geodesk
