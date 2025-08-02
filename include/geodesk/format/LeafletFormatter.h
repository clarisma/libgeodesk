// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "FeatureFormatter.h"
#include <geodesk/geom/polygon/Polygonizer.h>
#include <geodesk/geom/polygon/Ring.h>

namespace geodesk {

class LeafletFormatter : public FeatureFormatter<LeafletFormatter>
{
public:
    LeafletFormatter()
    {
        latitudeFirst_ = true;
    }

    void writeHeader(clarisma::Buffer& out, const char* extraStyles = nullptr);
    void writeFooter(clarisma::Buffer& out, const Box& bounds);

    void writeNodeGeometry(clarisma::Buffer& out, NodePtr node) const
    {
        out.write("L.circleMarker(");
        write(out, node.xy());
    }

    void writeWayGeometry(clarisma::Buffer& out, WayPtr way) const
    {
        out.write(way.isArea() ? "L.polygon(" : "L.polyline(");
        writeWayCoordinates(out, way, way.isArea());
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


private:
    const char* basemapUrl_ = "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
    const char* attribution_ = "Map data &copy; <a href=\"http://openstreetmap.org\">OpenStreetMap</a> contributors";
    const char* leafletUrl_ = "https://unpkg.com/leaflet@{leaflet_version}/dist/leaflet.js";
    const char* leafletStylesheetUrl_ = "https://unpkg.com/leaflet@{leaflet_version}/dist/leaflet.css";
    const char* leafletVersion_ = "1.8.0";
    std::string_view defaultMarkerStyle_;
    int minZoom_ = 0;
    int maxZoom_ = 19;
};

} // namespace geodesk