// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/DynamicStackBuffer.h>

namespace clarisma {

class StringBuilder : public DynamicStackBuffer<1024>
{
public:
	std::string toString() const
	{
		return { buf_, length() };
	}
};

} // namespace clarisma
