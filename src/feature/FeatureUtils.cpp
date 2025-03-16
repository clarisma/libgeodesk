// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/FeatureUtils.h>
#include <clarisma/text/Format.h>
#include <clarisma/util/StringBuilder.h>
#include <geodesk/feature/FeatureIterator.h>
#include <geodesk/feature/Tags.h>
#include <geodesk/feature/View.h>

using namespace clarisma;

namespace geodesk {

uint64_t FeatureUtils::countWorld(const View &view)
{
    uint64_t count = 0;
    Query query(view.store(), view.bounds(),
        view.types(), view.matcher(), view.filter());
    while (!query.next().isNull()) count++;
    return count;
}

uint64_t FeatureUtils::countGeneric(const View &view)
{
    uint64_t count = 0;
    FeatureIterator<Feature> iter(view);
    while (iter != nullptr)
    {
        ++iter;
        ++count;
    }
    return count;
}

uint64_t FeatureUtils::count(const View &view)
{
    switch (view.view())
    {
    case View::EMPTY:
        return 0;
    case View::WAY_NODES:
        if (!view.usesMatcherOrFilter())
        {
            return WayPtr(view.relatedFeature()).nodeCount();
        }
        break;
    case View::WORLD:
        return countWorld(view);
    default:
        break;
    }
    return countGeneric(view);
}

bool FeatureUtils::isEmpty(const View& view)
{
    if(view.view() == View::EMPTY) return true;
    FeatureIterator<Feature> iter(view);
    return iter == nullptr;
}

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
