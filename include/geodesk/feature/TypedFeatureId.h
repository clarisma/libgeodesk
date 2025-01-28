// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/types.h>
#include <clarisma/compile/unreachable.h>
#include <clarisma/text/Format.h>

namespace geodesk {

/// \cond lowlevel
class TypedFeatureId
{
public:
	explicit constexpr TypedFeatureId(uint64_t typedId) noexcept : typedId_(typedId) {}

	static constexpr TypedFeatureId ofTypeAndId(FeatureType type, uint64_t id) noexcept
	{
		return TypedFeatureId((id << 2) | static_cast<int>(type));
	}

	static constexpr TypedFeatureId ofNode(uint64_t id) noexcept
	{
		return TypedFeatureId(id << 2);
	}

	static constexpr TypedFeatureId ofWay(uint64_t id) noexcept
	{
		return ofTypeAndId(FeatureType::WAY, id);
	}

	static constexpr TypedFeatureId ofRelation(uint64_t id) noexcept
	{
		return ofTypeAndId(FeatureType::RELATION, id);
	}

	explicit operator uint64_t() const noexcept
	{
		return typedId_;
	}

	explicit operator int64_t() const noexcept
	{
		return static_cast<int64_t>(typedId_);
	}

	uint64_t id() const noexcept { return typedId_ >> 2; }
	FeatureType type() const noexcept { return static_cast<FeatureType>(typedId_ & 3); }
	bool isNode() const { return type() == FeatureType::NODE; }
	bool isWay() const { return type() == FeatureType::WAY; }
	bool isRelation() const { return type() == FeatureType::RELATION; }

	// TODO: will change in 2.0
	uint64_t asIdBits() const noexcept
	{
		uint64_t hi = (typedId_ >> 34) << 8;
		uint64_t lo = (typedId_ >> 2) << 32;
		return hi | lo | (static_cast<int>(type()) << 3);
	}
	 
	char* format(char* buf) const
	{
		std::string_view s = typeName(type());
		size_t size = s.size();
		memcpy(buf, s.data(), size);
		buf[size] = '/';
		return clarisma::Format::integer(buf + size + 1, id());
	}

	std::string toString() const
	{
		char buf[32];
		format(buf);
		return std::string(buf);
	}

	bool operator==(const TypedFeatureId&) const = default;
	bool operator!=(const TypedFeatureId&) const = default;

	bool operator<(const TypedFeatureId& other) const noexcept
	{
		uint64_t collateThis = typedId_ | ((typedId_ & 3) << 62);
		uint64_t collateOther = other.typedId_ | ((other.typedId_ & 3) << 62);
		return collateThis < collateOther;
	}

private:
	uint64_t typedId_;
};

template<typename Stream>
Stream&& operator<<(Stream&& out, TypedFeatureId tid)
{
	char buf[32];
	char* p = tid.format(buf);
	assert(p-buf < sizeof(buf));
	out.write(buf, p-buf);
	return std::forward<Stream>(out);
}

} // namespace geodesk

template<>
struct std::hash<geodesk::TypedFeatureId>
{
	size_t operator()(const geodesk::TypedFeatureId& tid) const noexcept
	{
		return std::hash<uint64_t>()(static_cast<uint64_t>(tid));
	}
};

// \endcond

