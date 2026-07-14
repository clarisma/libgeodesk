#pragma once

#include <geodesk/filter/Filter.h>

namespace geodesk {

class RoleFilter : public Filter
{
public:
	RoleFilter(std::span<std::string_view> roles)
	{
		// TODO
	}

	bool accept(FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const override
	{
		return false;
	}

	virtual bool acceptRole(int roleRode, const clarisma::ShortVarString* roleStr)
	{
		return true;	// TODO
	}
};

} // namespace geodesk
