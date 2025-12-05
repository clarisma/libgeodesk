// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "FeatureFormatter.h"
#include <geodesk/format/LeafletSettings.h>
#include <geodesk/geom/polygon/Polygonizer.h>
#include <geodesk/geom/polygon/Ring.h>

// \cond

namespace geodesk {

class LeafletFormatter : public FeatureFormatter<LeafletFormatter>
{
public:
    LeafletFormatter()
    {
        latitudeFirst_ = true;
    }

    void writeHeader(clarisma::Buffer& out, const LeafletSettings& settings,
        const char* extraStyles = nullptr);
    void writeFooter(clarisma::Buffer& out, const Box& bounds);

    void writeNodeGeometry(clarisma::Buffer& out, NodePtr node) const
    {
        out.write("L.circleMarker(");
        write(out, node.xy());
        out << ",style";
    }

    void writeWayGeometry(clarisma::Buffer& out, WayPtr way) const
    {
        out.write(way.isArea() ? "L.polygon(" : "L.polyline(");
        writeWayCoordinates(out, way, way.isArea());
        out << ",style";
    }

    void writeAreaRelationGeometry(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const
    {
        Polygonizer polygonizer;
        polygonizer.createRings(store, rel);
        polygonizer.assignAndMergeHoles();
        if (polygonizer.outerRings())    [[likely]]
        {
            out.write("L.polygon(");
            writePolygonizedCoordinates(out, polygonizer);
        }
        else
        {
            out.write("L.circleMarker(");
            write(out, rel.bounds().center());
        }
        out << ",style";
    }

    void writeCollectionRelationGeometry(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const
    {
        out.write("L.featureGroup([");
        uint64_t count = 0;
        bool isFirst = true;
        FastMemberIterator iter(store, rel);
        for (;;)
        {
            FeaturePtr member = iter.next();
            if (member.isNull()) break;
            if (!isFirst) out.writeByte(',');
            isFirst = false;
            int memberType = member.typeCode();
            if (memberType == 1)
            {
                self().writeWayGeometry(out, WayPtr(member));
            }
            else if (memberType == 0)
            {
                self().writeNodeGeometry(out, NodePtr(member));
            }
            else
            {
                assert(memberType == 2);
                self().writeRelationGeometry(out, store, RelationPtr(member));
            }
            out.writeByte(')');
        }
        out.writeByte(']');
    }

    void writeBox(clarisma::Buffer& out, const Box& box);

    static void writeSetColor(clarisma::Buffer& out, std::string_view color)
    {
        out << "style={color:'" << color << "'};\n";
    }

private:
    static constexpr std::string_view POINT_FUNCTION_STUB = "addpt(";
    static constexpr std::string_view LINE_FUNCTION_STUB = "addline(";
    static constexpr std::string_view POLYGON_FUNCTION_STUB = "addpoly(";
    static constexpr std::string_view COLLECION_FUNCTION_STUB = "addcoll(";
};

} // namespace geodesk

// \endcond