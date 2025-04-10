// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/format/FeatureWriter.h>
#include <geodesk/format/StringHolder.h>

namespace geodesk {
class AbstractTagIterator;
}

namespace geodesk {

class KeySchema;

///
/// \cond lowlevel
///
class CsvWriter : public FeatureWriter
{
public:
	explicit CsvWriter(clarisma::Buffer* buf, const KeySchema& keys);

	void writeFeature(FeatureStore* store, FeaturePtr feature) override;
	void writeAnonymousNodeNode(Coordinate point) override;
	void writeHeader() override;
	void writeFooter() override;

protected:
	void writeNodeGeometry(NodePtr node) override;
	void writeWayGeometry(WayPtr way) override;
	void writeAreaRelationGeometry(FeatureStore* store, RelationPtr relation) override;
	void writeCollectionRelationGeometry(FeatureStore* store, RelationPtr relation) override;

private:
	void setColumnValue(int col, const AbstractTagIterator& iter);

	const KeySchema& keys_;
	std::vector<StringHolder> columnValues_;
};

// \endcond
} // namespace geodesk
