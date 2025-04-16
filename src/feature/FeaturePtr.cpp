// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/FeaturePtr.h>
#include <geodesk/feature/FeatureUtils.h>

namespace geodesk {

const char* FeaturePtr::typeName() const
{
	int type = typeCode();
	if (type == 0) return "node";
	if (type == 1) return "way";
	if (type == 2) return "relation";
	return "invalid";
}


std::string FeaturePtr::toString() const
{
	char buf[64];
	FeatureUtils::format(buf, typeName(), id());
	return std::string(buf);
}



} // namespace geodesk
