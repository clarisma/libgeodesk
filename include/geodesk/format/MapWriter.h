// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/format/GeometryWriter.h>

namespace geodesk {

class MapWriter : public GeometryWriter
{
public:
    explicit MapWriter(clarisma::Buffer* buf) :
        GeometryWriter(buf)
    {
        latitudeFirst_ = true;
    }

    ~MapWriter() = default;

    void writeHeader();
    void writeFooter();
    void beginPoint(Coordinate c);
    void beginBox(const Box& box);
    void writePolygonOrPolyline(bool polygon);
    /*
    void writeWay(WayPtr way);
    void writeRelation(FeatureStore* store, RelationPtr relation);
    void writeRelationMembers(FeatureStore* store, RelationPtr relation, RecursionGuard& guard);
    bool writeFeature(PyFeature* feature);
    void writeGeometry(GEOSContextHandle_t context, const GEOSGeometry* geom);
    void writeGeometryCollection(GEOSContextHandle_t context, const GEOSGeometry* multi);
    void writeAttributeValue(PyObject* value);
    void formatAttributeValue(int key, PyObject* value);
    */

private:
    Box bounds_;
    const char* basemapUrl_ = "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
    const char* attribution_ = "Map data &copy; <a href=\"http://openstreetmap.org\">OpenStreetMap</a> contributors";
    const char* leafletUrl_ = "https://unpkg.com/leaflet@{leaflet_version}/dist/leaflet.js";
    const char* leafletStylesheetUrl_ = "https://unpkg.com/leaflet@{leaflet_version}/dist/leaflet.css";
    const char* leafletVersion_ = "1.8.0";
    int minZoom_ = 0;
    int maxZoom_ = 19;
};

} // namespace geodesk