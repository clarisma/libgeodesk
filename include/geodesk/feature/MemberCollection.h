// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <vector>
#include <geodesk/feature/RelationPtr.h>

namespace geodesk {

// TODO: not used, remove
/// \cond lowlevel
///
class MemberCollection : public std::vector<FeaturePtr>
{
public:
	MemberCollection(FeatureStore* store, RelationPtr relation);

	enum
	{
		PUNTAL = 1,
		LINEAL = 2,
		POLYGONAL = 4,
	};

private:
	void collect(FeatureStore* store, RelationPtr relation, RecursionGuard& guard);

	int types_;
};

// \endcond

} // namespace geodesk
