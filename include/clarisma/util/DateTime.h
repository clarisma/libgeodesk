// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <chrono>
#include <istream>
#include <clarisma/util/StringViewBuffer.h>

// TODO: Use std::time_t instead?

namespace clarisma {

class DateTime
{
public:
	DateTime() : timestamp_(0) {}	// NOLINT: implicit conv ok
	explicit DateTime(int64_t millisecondsSinceEpoch) : timestamp_(millisecondsSinceEpoch) {}
	explicit DateTime(uint64_t millisecondsSinceEpoch) : timestamp_(
		static_cast<int64_t>(millisecondsSinceEpoch)) {}

	DateTime(std::string_view s, const char* format)
	{
		using namespace std::chrono;

		StringViewBuffer buf(s);
		std::istream in(&buf);
		sys_time<milliseconds> tp;
		in >> parse(format, tp);
		timestamp_ = duration_cast<milliseconds>(tp.time_since_epoch()).count();
	}

	// TODO: Should 0 really be "null" as it is a valid timestamp?
	bool isNull() const
	{
		return timestamp_ == 0;
	}

	static DateTime now()
	{
		auto now = std::chrono::system_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
		return DateTime(duration.count());
	}

	operator int64_t() const { return timestamp_; }		// NOLINT: implicit conv ok
	void format(char* buf, size_t bufSize, const char* format) const;
	void format(char* buf) const;
	
private:
	int64_t timestamp_;
};

template<typename Stream>
Stream& operator<<(Stream& out, DateTime dt)
{
	char buf[32];
	dt.format(buf);
	std::string_view sv = buf;
	out.write(sv.data(), sv.size());
	return static_cast<Stream&>(out);
}

} // namespace clarisma
