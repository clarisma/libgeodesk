// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/text/Format.h>
#include <clarisma/util/streamable.h> // for << operator support
#include <geodesk/feature/Tex.h>
#include <geodesk/feature/Tip.h>

namespace geodesk {

using clarisma::operator<<;

/// \cond lowlevel
///
struct ForeignFeatureRef
{
	constexpr ForeignFeatureRef(Tip tip_, Tex tex_) : tip(tip_), tex(tex_) {}
	constexpr ForeignFeatureRef() : tip(0), tex(0) {}

	bool isNull() const { return tip.isNull(); }

	char* format(char* buf) const
	{
		tip.format(buf);
		buf[6] = '#';
		return clarisma::Format::integer(&buf[7], tex);
	}

	template<typename Stream>
	void format(Stream& out) const
	{
		char buf[32];
		char* p = format(buf);
		assert(p - buf < sizeof(buf));
		out.write(buf, p-buf);
	}

	std::string toString() const
	{
		char buf[32];
		format(buf);
		return std::string(buf);
	}

	bool operator==(const ForeignFeatureRef& other) const
	{
		return tip == other.tip && tex == other.tex;
	}

	Tip tip;
	Tex tex;
};

// \endcond


} // namespace geodesk
