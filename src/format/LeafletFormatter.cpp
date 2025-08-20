// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/format/LeafletFormatter.h>
#include <clarisma/text/Format.h>

using namespace clarisma;

namespace geodesk {

void LeafletFormatter::writeBox(clarisma::Buffer& out, const Box& box)
{
	out << "L.rectangle([[";
	write(out, box.topLeft());
	out << "],[";
	write(out, box.bottomRight());
	out << "]]";
}

void LeafletFormatter::writeHeader(Buffer& out, const LeafletSettings& settings, const char* extraStyles)
{
	out <<
	  "<html><head><meta charset=\"utf-8\">"
	  "<link rel=\"stylesheet\" href=\"";
	Format::writeReplacedString(out, settings.leafletStylesheetUrl, "{leaflet_version}", settings.leafletVersion);
	out << "\">\n<script src=\"";
	Format::writeReplacedString(out, settings.leafletUrl, "{leaflet_version}", settings.leafletVersion);
	out <<  "\"></script>\n<style>\n#map {height: 100%;}\nbody {margin:0;}\n";
	if(extraStyles) out << extraStyles;
	out << "</style>\n</head>\n<body>\n<div id=\"map\">"
		"<a class='logo' href='https://docs.geodesk.com/gol/map' target='_blank'></a></div>\n<script>"
		"var map = L.map('map');\n"
		"var tilesUrl='";
	out << settings.basemapUrl;
	out << "';\nvar tilesAttrib='";
	out << settings.attribution;
	out <<
		"';\nvar tileLayer = new L.TileLayer("
		"tilesUrl, {minZoom: " << settings.minZoom
		<< ", maxZoom: " << settings.maxZoom <<
		", attribution: tilesAttrib});\n"
		"map.setView([51.505, -0.09], 13);\n"      // TODO
		"map.addLayer(tileLayer);\n"
		"map.zoomControl.setPosition('topright');\n"
		"L.control.scale().addTo(map);\n"
		"var style = {};\n";
		/*
		"function " << POINT_FUNCTION_STUB << "c) { "
		"L.circleMarker(c,style).addTo(map); }\n"
		"function " << LINE_FUNCTION_STUB << "c) { "
		"L.polyline(c,style).addTo(map); }\n"
		"function " << POLYGON_FUNCTION_STUB << "c) { "
		"L.polygon(c,style).addTo(map); }\n"
		"function " << COLLECION_FUNCTION_STUB << "c) { "
		"L.circleMarker(c,style).addTo(map); }\n";
		*/
}

void LeafletFormatter::writeFooter(clarisma::Buffer& out, const Box& bounds)
{
	out << "map.fitBounds([[";
	write(out, bounds.bottomLeft());
	out << "],[";
	write(out, bounds.topRight());
	out << "]]);"
		"</script></body></html>";
}

} // namespace geodesk