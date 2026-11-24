// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdint>
#include <regex>
#include <string_view>
#include <clarisma/util/DataPtr.h>
#include <clarisma/util/ShortVarString.h>
#include <geodesk/match/Matcher.h>

namespace geodesk {


class MatcherEngine
{
public:
	static int accept(const Matcher*, FeaturePtr);

private:
	void jumpIf(int matched) { ip_ += matched ? ip_.getShort() : 2; }
	inline int scanGlobalKeys();
	inline int scanLocalKeys();
		// This is a cold method that we normally wouldn't inline,
		// but inlining prevents escape of `ctx` and help keep it in regs
	static int isNegated(int op) { return (op >> 8) & 1; }
		// TODO: Is negate the only flag? If so, no need for AND
	inline std::string_view getStringOperand();
	static std::string_view asStringView(const uint8_t* stringValue)
	{
		const clarisma::ShortVarString* val =
			reinterpret_cast<const clarisma::ShortVarString*>(stringValue);
		return val->toStringView();
	}
	inline double getDoubleOperand();
	inline const std::regex* getRegexOperand();
	inline uint32_t getFeatureTypeOperand();

	clarisma::DataPtr ip_;
	const uint8_t* pTagTable_;	// with local-keys flag as bit 0
	clarisma::DataPtr pTag_;
	uint16_t tagKey_;
	int16_t valueOfs_;
};


} // namespace geodesk
