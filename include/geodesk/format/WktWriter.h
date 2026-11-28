// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/format/FeatureWriter.h>

namespace geodesk {

///
/// \cond lowlevel
///
class WktWriter : public FeatureWriter
{
public:
	explicit WktWriter(clarisma::Buffer* buf);

	void writeFeature(const FeatureStore* store, FeaturePtr feature) override;
	void writeAnonymousNodeNode(Coordinate point) override;
	void writeHeader() override;
	void writeFooter() override;

protected:
	void writeNodeGeometry(NodePtr node) override;
	void writeWayGeometry(WayPtr way) override;
	void writeAreaRelationGeometry(const FeatureStore* store, RelationPtr relation) override;
	void writeCollectionRelationGeometry(const FeatureStore* store, RelationPtr relation) override;
};

// \endcond
} // namespace geodesk
