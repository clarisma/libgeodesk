// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/text/Format.h>
#include <clarisma/util/streamable.h> // for << operator support

#include "StringBuilder.h"

namespace clarisma {

class FileVersion
{
public:
	constexpr FileVersion(int major, int minor) :
		major_(static_cast<uint16_t>(major)),
		minor_(static_cast<uint16_t>(minor)) {}

	uint16_t major() const noexcept { return major_; }
	uint16_t minor() const noexcept { return minor_; }

	char* formatReverse(char* end) const
	{
		end = Format::unsignedIntegerReverse(minor_, end);
		--end;
		*end = '.';
		return Format::unsignedIntegerReverse(major_, end);
	}

	char* format(char* buf) const
	{
		char tmp[32];
		char* end = tmp + sizeof(tmp);
		char* start = formatReverse(end);
		size_t len = end - start;
		memcpy(buf, start, len);
		*(buf + len) = 0;
		return buf + len;
	}

	template<typename Stream>
	void format(Stream& out) const
	{
		char buf[32];
		char* end = buf + sizeof(buf);
		char* start = formatReverse(end);
		out.write(start, end - start);
	}

	explicit operator uint32_t() const
	{
		return static_cast<uint32_t>(major_) << 16 | minor_;
	}

	/*
	constexpr bool operator==(FileVersion other) const noexcept
	{
		return major_ == other.major_ && minor_ == other.minor_;
	}

	constexpr bool operator!=(FileVersion other) const noexcept
	{
		return !(*this == other);
	}
	*/

	constexpr bool operator==(const FileVersion& other) const = default;

	auto operator<=>(const FileVersion& other) const
	{
		return static_cast<const uint32_t>(*this) <=>
			static_cast<const uint32_t>(other);
	}

	void checkExact(const char* type, FileVersion required) const
	{
		if (*this != required)
		{
			StringBuilder s;
			s << "Unsupported " << type << " version " << *this
				<< " (required: " << required << ")";
			throw std::runtime_error(s.toString());
		}
	}

private:
	uint16_t major_;
	uint16_t minor_;
};

} // namespace clarisma
