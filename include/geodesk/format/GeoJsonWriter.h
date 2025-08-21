// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/format/FeatureWriter.h>
#include <geodesk/feature/TagIterator.h>

namespace geodesk {

#ifdef CLARISMA_KEEP_BUFFERWRITER

///
/// \cond lowlevel
///
class GeoJsonWriter : public FeatureWriter
{
public:
	explicit GeoJsonWriter(clarisma::Buffer* buf) :
		FeatureWriter(buf),
		linewise_(false)
	{
	}

	void linewise(bool b) { linewise_ = b; }

	void writeFeature(FeatureStore* store, FeaturePtr feature) override;
	void writeAnonymousNodeNode(Coordinate point) override;
	void writeHeader() override;
	void writeFooter() override;

	void writeTags(TagIterator& iter);

protected:
	// void writeId(FeatureRef feature);
	void writeNodeGeometry(NodePtr node) override;
	void writeWayGeometry(WayPtr way) override;
	void writeAreaRelationGeometry(FeatureStore* store, RelationPtr relation) override;
	void writeCollectionRelationGeometry(FeatureStore* store, RelationPtr relation) override;

	bool linewise_;
};

// \endcond

} // namespace geodesk

#endif