// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstdint>
#include <clarisma/util/Buffer.h>

namespace geodesk {

class FeatureStore;

class MatcherDecoder
{
public:
	MatcherDecoder(FeatureStore* store, clarisma::Buffer& out, const uint16_t* pCode) :
		store_(store), out_(out), pCodeStart_(pCode), pLastInstruction_(pCode) {}

	void decode();
	
private:
	void writeAddress(const uint16_t* p, bool padded);
	void writeOpcodeStub(const uint16_t* p);
	void writeBranchingOp(const uint16_t* p);

	const uint16_t* pCodeStart_;
	const uint16_t* pLastInstruction_;
	clarisma::Buffer& out_;
	FeatureStore* store_;
};

} // namespace geodesk
