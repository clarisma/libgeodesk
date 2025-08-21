// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef CLARISMA_KEEP_BUFFERWRITER

#include <geodesk/format/MapWriter.h>

namespace geodesk {

void MapWriter::writeHeader(const char* extraStyles)
{
	writeConstString(
	  "<html><head><meta charset=\"utf-8\">"
	  "<link rel=\"stylesheet\" href=\"");
	writeReplacedString(leafletStylesheetUrl_, "{leaflet_version}", leafletVersion_);
	writeConstString("\">\n<script src=\"");
	writeReplacedString(leafletUrl_, "{leaflet_version}", leafletVersion_);
	writeConstString("\"></script>\n<style>\n#map {height: 100%;}\nbody {margin:0;}\n");
	if(extraStyles) writeString(extraStyles);
	writeConstString("</style>\n</head>\n<body>\n<div id=\"map\"> </div>\n"
		"<script>");

	writeConstString(
		"var map = L.map('map');\n"
		"var tilesUrl='");
	writeString(basemapUrl_);
	writeConstString("';\nvar tilesAttrib='");
	writeString(attribution_);
	writeConstString(
		"';\nvar tileLayer = new L.TileLayer("
		"tilesUrl, {minZoom: ");
	formatInt(minZoom_);
	writeConstString(", maxZoom: ");
	formatInt(maxZoom_);
	writeConstString(
		", attribution: tilesAttrib});\n"
		"map.setView([51.505, -0.09], 13);\n"      // TODO
		"map.addLayer(tileLayer);\n"
		"L.control.scale().addTo(map);\n");
}

void MapWriter::writeFooter()
{
	writeConstString("map.fitBounds([");
	writeCoordinate(Coordinate(bounds_.minX(), bounds_.minY()));
	writeConstString(",");
	writeCoordinate(Coordinate(bounds_.maxX(), bounds_.maxY()));
	writeConstString("]);");
	writeConstString("</script></body></html>");
	flush();
}


void MapWriter::beginPoint(Coordinate c)
{
	writeConstString("L.circle(");
	writeCoordinate(c);
}

void MapWriter::beginBox(const Box& box)
{
	writeConstString("L.rectangle([[");
	writeCoordinate(box.topLeft());
	writeConstString("],[");
	writeCoordinate(box.bottomRight());
	writeConstString("]]");
}

void MapWriter::writePolygonOrPolyline(bool polygon)
{
	if (polygon)
	{
		writeConstString("L.polygon(");
	}
	else
	{
		writeConstString("L.polyline(");
	}
}

/*
void MapWriter::writeWay(WayPtr way)
{
	writePolygonOrPolyline(way.isArea());
	writeWayCoordinates(way, false);
	// TODO: Leaflet doesn't need duplicate end coordinate for polygons
}

// TODO: don't allow placeholders/empty rels
void MapWriter::writeRelation(FeatureStore* store, RelationPtr relation)
{
	if (relation.isArea())
	{
		Polygonizer polygonizer;
		polygonizer.createRings(store, relation);
		polygonizer.assignAndMergeHoles();
		if (!polygonizer.outerRings())
		{
			writePoint(relation.bounds().center());
		}
		else
		{
			writePolygonOrPolyline(true);
			writePolygonizedCoordinates(polygonizer);
		}
	}
	else
	{
		writeConstString("L.featureGroup([");
		RecursionGuard guard(relation);
		writeRelationMembers(store, relation, guard);
		writeByte(']');
	}

}

void MapWriter::writeRelationMembers(FeatureStore* store, RelationPtr relation, RecursionGuard& guard)
{
	bool first = true;
	FastMemberIterator iter(store, relation);
	for (;;)
	{
		FeaturePtr member = iter.next();
		if (member.isNull()) break;
		int memberType = member.typeCode();
		if (memberType == 1)
		{
			WayPtr memberWay(member);
			if (memberWay.isPlaceholder()) continue;
			if (!first) writeByte(',');
			writeWay(memberWay);
		}
		else if (memberType == 0)
		{
			NodePtr memberNode(member);
			if (memberNode.isPlaceholder()) continue;
			if (!first) writeByte(',');
			writePoint(memberNode.xy());
		}
		else
		{
			RelationPtr childRel(member);
			if (childRel.isPlaceholder() || !guard.checkAndAdd(childRel)) continue;
			if (!first) writeByte(',');
			writeRelation(store, childRel);
		}
		writeByte(')');
		first = false;
	}
}


bool MapWriter::writeFeature(PyFeature* obj)
{
	FeaturePtr feature = obj->feature;
	if (feature.isWay())
	{
		WayPtr way(feature);
		if (way.isPlaceholder()) return false;
		writeWay(way);
		bounds_.expandToIncludeSimple(way.bounds());
	}
	else if (feature.isNode())
	{
		NodePtr node(feature);
		if (node.isPlaceholder()) return false;
		writePoint(node.xy());
		bounds_.expandToInclude(node.xy());
	}
	else
	{
		assert(feature.isRelation());
		RelationPtr relation(feature);
		if (relation.isPlaceholder()) return false;
		writeRelation(obj->store, relation);
		bounds_.expandToIncludeSimple(relation.bounds());
	}
	return true;
}

void MapWriter::writeItem(PyMap::Element* item)
{
	schema_.fill(map_, item);
	if (schema_.hasFormatters) initBinder();
	PyObject* obj = item->object;
	writeObject(obj);
	schema_.clear();
}

void MapWriter::writeGeometry(GEOSContextHandle_t context, const GEOSGeometry* geom)
{
	int geomType = GEOSGeomTypeId_r(context, geom);
	switch (geomType)
	{
	case GEOS_POINT:
		writeConstString("L.circle(");
		writePointCoordinates(context, geom);
		return;
	case GEOS_LINESTRING:
	case GEOS_LINEARRING:
		writeConstString("L.polyline(");
		writeLineStringCoordinates(context, geom);
		return;
	case GEOS_POLYGON:
		writeConstString("L.polygon(");
		writePolygonCoordinates(context, geom);
		return;
	case GEOS_MULTIPOLYGON:
		writeConstString("L.polygon(");
		writeMultiPolygonCoordinates(context, geom);
		return;
	}

	// Geometry collection (other than MultiPolygon)
	writeGeometryCollection(context, geom);
}


void MapWriter::writeGeometryCollection(GEOSContextHandle_t context, const GEOSGeometry* multi)
{
	writeConstString("L.featureGroup([");
	int count = GEOSGetNumGeometries_r(context, multi);
	for (int i = 0; i < count; i++)
	{
		if (i > 0) writeByte(',');
		writeGeometry(context, GEOSGetGeometryN_r(context, multi, i));
		writeByte(')');
	}
	writeByte(']');
}

void MapWriter::writeObject(PyObject* obj)
{
	PyTypeObject* type = Py_TYPE(obj);
	geos::geom::Geometry* geom;

	if (type == &PyFeature::TYPE)
	{
		if (!writeFeature((PyFeature*)obj)) return;
	}
	else if (type == &PyCoordinate::TYPE)
	{
		PyCoordinate* coordObj = (PyCoordinate*)obj;
		Coordinate c(coordObj->x, coordObj->y);
		writePoint(c);
		bounds_.expandToInclude(c);
	}
	else if (type == &PyBox::TYPE)
	{
		PyBox* boxObj = (PyBox*)obj;
		writeBox(boxObj->box);
		bounds_.expandToIncludeSimple(boxObj->box);
	}
	else if (type == &PyAnonymousNode::TYPE)
	{
		PyAnonymousNode* anonNode = (PyAnonymousNode*)obj;
		Coordinate c(anonNode->x_, anonNode->y_);
		writePoint(c);
		bounds_.expandToInclude(c);
	}
	else if (Python::isIterable(obj))
	{
		PyObject* iter = PyObject_GetIter(obj);
		PyObject* childObj;
		while ((childObj = PyIter_Next(iter)))
		{
			writeObject(childObj);
			Py_DECREF(childObj);
		}
		return;		// don't continue to attributes/tooltip/link
	}
	else if (Environment::get().getGeosGeometry(obj, (GEOSGeometry**)&geom))
	{
		GEOSContextHandle_t context = Environment::get().getGeosContext();
		writeGeometry(context, (GEOSGeometry*)geom);
		bounds_.expandToIncludeSimple(Box(geom->getEnvelopeInternal()));
	}
	else
	{
		// TODO: invalid arg, throw
		return;
	}

	if (schema_.attrCount > 0)
	{
		writeConstString(",{");
		for (int i = 0; i < schema_.attrCount; i++)
		{
			if (i > 0) writeByte(',');
			const PyMap::ElementAttribute* attr = &schema_.attributes[i];
			writeString(PyMap::ATTR_NAMES[attr->key]);
			writeByte(':');
			formatAttributeValue(attr->key, attr->value);
		}
		writeByte('}');
	}
	writeByte(')');

	if (schema_.hasFormatters) binder_->addTarget(obj);
	if (schema_.tooltip)
	{
		writeConstString(".bindTooltip(");
		formatAttributeValue(PyMap::TOOLTIP, schema_.tooltip);
		writeConstString(", {sticky: true})");
	}
	if (schema_.link)
	{
		writeConstString(".on('click', function(){window.location=");
		formatAttributeValue(PyMap::LINK, schema_.link);
		writeConstString(";})");
	}
	writeConstString(".addTo(map);\n");
	if (schema_.hasFormatters) binder_->popTarget();
}

void MapWriter::writeAttributeValue(PyObject* value)
{
	if (PyUnicode_Check(value))
	{
		writeByte('\"');
		writeJsonEscapedString(Python::stringAsStringView(value));
			// TODO: this throws!
		writeByte('\"');
	}
	else if (value == Py_True)
	{
		writeConstString("true");
	}
	else if (value == Py_False)
	{
		writeConstString("false");
	}
	else
	{
		PyObject* str = PyObject_Str(value);
		if (str)
		{
			const char* s = PyUnicode_AsUTF8(str);
			if (s) writeString(s);
		}
		else
		{
			// Ignore error and write dummy string (to keep script valid)
			PyErr_Clear();
			writeString("\"\"");
		}
	}
}


void MapWriter::formatAttributeValue(int key, PyObject* value)
{
	if (PyCallable_Check(value))
	{
		// PyObject* formatted = PyObject_CallOneArg(value, (PyObject*)binder_);
		//  works only for Python 3.9+
		PyObject* formatted = PyObject_CallFunctionObjArgs(value, (PyObject*)binder_, NULL);
		if (!formatted)
		{
			PyErr_Clear();
			writeString("\"\"");
			return;
		}
		int flags = PyMap::ATTR_FLAGS[key];
		if (flags & (PyMap::AttributeFlags::NUM_VALUE | PyMap::AttributeFlags::BOOL_VALUE))
		{
			if (flags & (PyMap::AttributeFlags::NUM_VALUE))
			{
				PyObject* numValue = PyFloat_FromString(formatted);
				if (numValue == NULL)
				{
					PyErr_Clear();
					// If the formatted string does not represent a valid number,
					// write the default value
					writeString(PyMap::ATTR_DEFAULTS[key]);
				}
				else
				{
					writeAttributeValue(numValue);
					Py_DECREF(numValue);
				}
			}
			else
			{
				// Boolean value
				const char* s = PyUnicode_AsUTF8(formatted);
				const char* boolString = "false";
				if (s == NULL)
				{
					PyErr_Clear();
				}
				else if (*s != 0 && strcmp(s, "False") != 0)
				{
					boolString = "true";
				}
				writeString(boolString);
			}
		}
		else
		{
			writeAttributeValue(formatted);
		}
		Py_DECREF(formatted);
		return;
	}
	else
	{
		writeAttributeValue(value);
	}
}

MapWriter::Schema::Schema()
{
	memset(this, 0, sizeof(*this));
}

// We don't need refcounting here because all objects are guaranteed
// to be live while we write the map data
void MapWriter::Schema::set(int key, PyObject* value)
{
	if (PyCallable_Check(value)) hasFormatters = true;
	switch (key)
	{
	case PyMap::TOOLTIP:
		tooltip = value;
		break;
	case PyMap::LINK:
		link = value;
		break;
	default:
		attributes[attrCount].key = key;
		attributes[attrCount].value = value;
		attrCount++;
		break;
	}
}

// TODO: element values could be null if add() failed due to invalid keywords!!!

void MapWriter::Schema::fill(const PyMap& map, const PyMap::Element* item)
{
	uint64_t specificElementAttributes = 0;
	const PyMap::ElementAttribute* p = item->attributes;
	const PyMap::ElementAttribute* end = p + item->attrCount;
	while (p < end)
	{
		int key = p->key;
		set(key, p->value);
		specificElementAttributes |= 1ULL << key;
		p++;
	}

	BitIterator<uint64_t> iter(map.globalElementAttributes);
	for (;;)
	{
		int key = iter.next();
		if (key < 0) break;
		if ((specificElementAttributes & (1ULL << key)) == 0)
		{
			set(key, map.attributes[key]);
		}
	}
}

 */

} // namespace geodesk

#endif