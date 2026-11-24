// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "OpGraph.h"
#include "Selector.h"
#include "TagClause.h"
#include <clarisma/util/Parser.h>
#include <geodesk/feature/GlobalStrings.h>
#include <geodesk/feature/Snapshot.h>

namespace geodesk {

class MatcherParser : public clarisma::Parser
{
public:
	MatcherParser(const Snapshot* snapshot, const char* pInput);

	Selector* parse();
	OpGraph& graph() { return graph_; }
	uint32_t indexBits() const { return indexBits_; }
	uint32_t keyMin() const { return keyMin_; }
	static constexpr int codeNo() { return GlobalStrings::NO; }

private:
	int getStringCode(std::string_view s) const
	{
		return snapshot_->strings().getCode(s.data(), s.length());
	}
	Selector* expectSelector();
	TagClause* expectTagClause();
	TagClause* expectKey();
	std::string_view acceptEscapedString();
	RegexOperand* expectRegex();
	OpNode* acceptStringOperand();
	FeatureTypes matchTypes();

	static const clarisma::CharSchema VALID_FIRST_CHAR;
	static const clarisma::CharSchema VALID_NEXT_CHAR;

	const Snapshot* snapshot_;
	OpGraph graph_;
	Selector* currentSel_;
	uint32_t indexBits_;		// TODO: rename to keyMask_
	uint32_t keyMin_ = 0xffff'ffff;
		// TODO: It would be better to break out the index bits based on feature type,
		// which would allow polyform queries to use indexes more efficiently
		// (e.g. "na[amenity=fire_station), n[emergency=fire_hydrant]" could
		// avoid selecting buckets with "emergency" keys but no "amenity" keys
		// when querying areas), but these types of queries are rare and this
		// design is simpler
};


} // namespace geodesk
