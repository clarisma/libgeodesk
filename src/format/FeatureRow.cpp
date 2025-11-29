// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: AGPL-3.0-only

#include <clarisma/util/Json.h>
#include <clarisma/util/StringBuilder.h>
#include <geodesk/format/FeatureRow.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/feature/TagWalker.h>
#include <geodesk/format/KeySchema.h>
#include <geodesk/format/WktFormatter.h>
#include <geodesk/geom/Centroid.h>

using namespace clarisma;

static void writeTag(StringBuilder& out, TagWalker& tw)
{
    out.writeByte('\"');
    Json::writeEscaped(out, tw.key()->toStringView());
    out.writeByte('\"');
    out.writeByte(':');
    if(tw.isStringValue())  [[likely]]
    {
        out.writeByte('\"');
        Json::writeEscaped(out, tw.stringValueFast()->toStringView());
        out.writeByte('\"');
    }
    else
    {
        out << tw.numberValueFast();
    }
}

FeatureRow::FeatureRow(const KeySchema& keys, FeatureStore* store,
    FeaturePtr feature, int precision,
    StringBuilder& stringBuilder) :
    SmallArray(keys.columnCount())
{
    char buf[32];
    stringBuilder.clear();

    size_t colCount = keys.columnCount();
    for (int i=0; i<colCount; i++)
    {
        (*this)[i] = StringHolder();
    }

    Coordinate xy;
    int idCol = keys.columnOfSpecial(KeySchema::ID);
    int lonCol = keys.columnOfSpecial(KeySchema::LON);
    int latCol = keys.columnOfSpecial(KeySchema::LAT);
    int tagsCol = keys.columnOfSpecial(KeySchema::TAGS);
    int geomCol = keys.columnOfSpecial(KeySchema::GEOM);

    if (idCol)
    {
        char* end = &buf[sizeof(buf)];
        char* p = Format::unsignedIntegerReverse(feature.id(), end);
        p--;
        *p = "NWRX"[feature.typeCode()];
        (*this)[idCol-1] = StringHolder::inlineCopy(p, end-p);
    }
    if ((lonCol | latCol)  != 0)
    {
        xy = Centroid::ofFeature(store, feature);
        if (lonCol)  [[likely]]
        {
            char* p = Format::formatDouble(buf,
                Mercator::lonFromX(xy.x), precision);
            (*this)[lonCol-1] = StringHolder::inlineCopy(buf, p-buf);
        }
        if (latCol)  [[likely]]
        {
            char* p = Format::formatDouble(buf,
                Mercator::latFromY(xy.y), precision);
            (*this)[latCol-1] = StringHolder::inlineCopy(buf, p-buf);
        }
    }

    TagWalker tw(feature.tags(), store->strings());
    while (tw.next())
    {
        int col;
        if(tw.keyCode() >= 0) [[likely]]
        {
            col = keys.columnOfGlobal(tw.keyCode());
        }
        else
        {
            col = keys.columnOfLocal(tw.key()->toStringView());
        }
        if (col > 0)
        {
            assert(col <= colCount);
            if (tw.isStringValue()) [[likely]]
            {
                (*this)[col-1] = StringHolder(tw.stringValueFast());
            }
            else
            {
                (*this)[col-1] = StringHolder(tw.numberValueFast());
            }
        }
        else if(col < 0 && tagsCol)        // wildcard
        {
            stringBuilder.writeByte(stringBuilder.isEmpty() ?
                '{' : ',');
            writeTag(stringBuilder, tw);
        }
    }

    size_t tagsSize = stringBuilder.length();
    if(tagsSize)
    {
        stringBuilder.writeByte('}');
        tagsSize++;

        // TODO: Decide if to write an empty object {}
        //  (rather than leave the column blank) if
        //  there are no additional tags
    }

    // Don't take the string_view yet, because the StringBuilder's
    // buffer may be relocated if we write the geometry

    size_t geomSize = 0;
    if(geomCol)
    {
        WktFormatter wkt;
        wkt.precision(precision);
        wkt.writeFeatureGeometry(stringBuilder, store, feature);
        geomSize = stringBuilder.length() - tagsSize;
    }

    if(tagsCol)
    {
        (*this)[tagsCol-1] = StringHolder(
            std::string_view(stringBuilder.data(), tagsSize));
    }

    if(geomCol)
    {
        (*this)[geomCol-1] = StringHolder(
            std::string_view(stringBuilder.data() + tagsSize,
                geomSize));
    }
}
