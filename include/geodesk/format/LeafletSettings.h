// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once


#include "FeatureFormatter.h"

// \cond

namespace geodesk {

struct LeafletSettings
{
    std::string_view basemapUrl = "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
    std::string_view attribution = "Map data &copy; <a href=\"http://openstreetmap.org\">OpenStreetMap</a> contributors";
    const char* leafletUrl = "https://unpkg.com/leaflet@{leaflet_version}/dist/leaflet.js";
    const char* leafletStylesheetUrl = "https://unpkg.com/leaflet@{leaflet_version}/dist/leaflet.css";
    const char* leafletVersion = "1.8.0";
    // std::string_view defaultMarkerStyle_;
    int minZoom = 0;
    int maxZoom = 19;
};

} // namespace geodesk

/// \endcond