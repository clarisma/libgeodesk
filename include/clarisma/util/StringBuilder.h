// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/StreamWriter.h>
#include <clarisma/util/DynamicStackBuffer.h>

namespace clarisma {

// TODO: deprecate, or derive directly from DynamicStackBuffer
class StringBuilder : public AbstractStreamWriter<StringBuilder>
{
public:
	explicit StringBuilder()
	{
		setBuffer(&buf_);
	}

	std::string toString()
	{
		flush();
		return { buf_.data(), buf_.length() };
	}

private:
	DynamicStackBuffer<1024> buf_;
};

} // namespace clarisma
