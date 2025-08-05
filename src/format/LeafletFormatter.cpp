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
	Format::writeReplacedString(out, settings.leafletStylesheetUrl, "{leaflet_version}", leafletVersion_);
	out << "\">\n<script src=\"";
	Format::writeReplacedString(out, settings.leafletUrl, "{leaflet_version}", leafletVersion_);
	out <<  "\"></script>\n<style>\n#map {height: 100%;}\nbody {margin:0;}\n";
	if(extraStyles) out << extraStyles;
	out << "</style>\n</head>\n<body>\n<div id=\"map\"></div>\n<script>"
		"var map = L.map('map');\n"
		"var tilesUrl='";
	out << settings.basemapUrl;
	out << "';\nvar tilesAttrib='";
	out << settings.attribution;
	out <<
		"';\nvar tileLayer = new L.TileLayer("
		"tilesUrl, {minZoom: " << minZoom_
		<< ", maxZoom: " << maxZoom_ <<
		", attribution: tilesAttrib});\n"
		"map.setView([51.505, -0.09], 13);\n"      // TODO
		"map.addLayer(tileLayer);\n"
		"L.control.scale().addTo(map);\n";
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