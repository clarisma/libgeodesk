// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef false

#include <geodesk/format/CsvWriter.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/feature/GlobalTagIterator.h>
#include <geodesk/feature/LocalTagIterator.h>
#include <geodesk/feature/Tags.h>
#include <geodesk/format/KeySchema.h>
#include <geodesk/geom/polygon/Polygonizer.h>
#include "geom/polygon/Ring.h"

namespace geodesk {

using namespace clarisma;

CsvWriter::CsvWriter(Buffer* buf, const KeySchema& keys) :
	FeatureWriter(buf),
	keys_(keys)
{
	coordValueSeparatorChar_ = ' ';
	coordStartChar_ = 0;
	coordEndChar_ = 0;
	coordGroupStartChar_ = '(';
	coordGroupEndChar_ = ')';
	columnValues_.reserve(keys.columnCount());
}


void CsvWriter::writeAnonymousNodeNode(Coordinate point)
{

}


void CsvWriter::writeNodeGeometry(NodePtr node)
{

}

void CsvWriter::writeWayGeometry(WayPtr way)
{
	if (way.isArea())
	{
		writeConstString("POLYGON");
	}
	else
	{
		writeConstString("LINESTRING");
	}
	writeWayCoordinates(way, way.isArea());
}

void CsvWriter::writeAreaRelationGeometry(FeatureStore* store, RelationPtr relation)
{
	Polygonizer polygonizer;
	polygonizer.createRings(store, relation);
	polygonizer.assignAndMergeHoles();
	const Polygonizer::Ring* ring = polygonizer.outerRings();
	int count = ring ? (ring->next() ? 2 : 1) : 0;
	if (count > 1)
	{
		writeConstString("MULTIPOLYGON");
	}
	else
	{
		writeConstString("POLYGON");
	}
	if (count == 0)
	{
		writeConstString(" EMPTY");
	}
	else
	{
		writePolygonizedCoordinates(polygonizer);
	}
}


void CsvWriter::writeCollectionRelationGeometry(FeatureStore* store, RelationPtr relation)
{
	writeConstString("GEOMETRYCOLLECTION");
	if (writeMemberGeometries(store, relation) == 0) writeConstString(" EMPTY");
}



void CsvWriter::writeFeature(FeatureStore* store, FeaturePtr feature)
{
	TagTablePtr tags = feature.tags();
	for (int i=0; i<columnValues_.size(); i++)
	{
		columnValues_[i] = StringHolder();
	}

	// TODO: Check if 0 handle can be used here

	GlobalTagIterator iterGlobal(0, tags);
	while (iterGlobal.next())
	{
		int col = keys_.columnOfGlobal(iterGlobal.key());
		if (col) setColumnValue(col, iterGlobal);
	}

	LocalTagIterator iterLocal(0, tags);
	while (iterLocal.next())
	{
		int col = keys_.columnOfLocal(iterLocal.keyString()->toStringView());
		if (col) setColumnValue(col, iterLocal);
	}
	firstFeature_ = false;
}

void CsvWriter::setColumnValue(int col, const AbstractTagIterator& iter)
{
	// if (col == KeySchema::WILDCARD)
}


void CsvWriter::writeHeader()
{
	bool firstCol = true;
	for (auto col : keys_.columns())
	{
		if (!firstCol) writeByte(',');
		writeString(col);
		firstCol = false;
	}
	writeByte('\n');
}

void CsvWriter::writeFooter()
{
}


} // namespace geodesk

#endif