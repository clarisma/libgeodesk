// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/filter/Filter.h>
#include <span>

namespace geodesk {

/// @brief Filter to select relation members that have a given role
///
/// The accepted global and local roles are stored inline
/// after the RoleFilter object, in the following format:
///
/// Zero or more:
///   uint16_t    Global-string code
/// Zero or more
///   uint8_t     Length of local string
///   char[]      UTF-8 of local string
///
/// Local role strings are capped to 255 bytes maximum
///
class RoleFilter : public Filter
{
public:
	RoleFilter(std::span<const std::string_view> roles, const StringTable& strings);
	RoleFilter(const RoleFilter&) = delete;
	RoleFilter(RoleFilter&&) = delete;

	~RoleFilter() override
	{
		if (data_ != &inlineData_[0])	[[unlikely]]
		{
			delete[] data_;
		}
	}

	bool accept(FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const override
	{
		// RoleFilter only accepts features in relation members queries;
		// those queries call acceptRole() instead.
		// All other queries call accept(), so we always return false
		return false;
	}

	virtual bool acceptRole(int roleCode, const clarisma::ShortVarString* roleStr) const;

private:
	static constexpr size_t INLINE_DATA_SIZE = 32;

	uint8_t* data_ = &inlineData_[0];
	int globalRoleCount_ = 0;
	int localRoleCount_ = 0;
	alignas(uint16_t) uint8_t inlineData_[INLINE_DATA_SIZE];
};

} // namespace geodesk
