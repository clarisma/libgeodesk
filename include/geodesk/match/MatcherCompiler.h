// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

// #define ASMJIT_STATIC 
// #include <asmjit/asmjit.h>
#include <cstdint>

namespace geodesk {

class MatcherHolder;
class OpGraph;
struct Selector;
class Snapshot;

/// \cond lowlevel

class MatcherCompiler
{
public:
	explicit MatcherCompiler(const Snapshot* snapshot) :
		snapshot_(snapshot)
	{
		// TODO: fix this dependency, store not initialized yet
	}

	const MatcherHolder* getMatcher(const char* query);

private:
	const MatcherHolder* compileMatcher(OpGraph& graph, Selector* firstSel,
		uint32_t indexBits, uint32_t keyMin);

	const Snapshot* snapshot_;
	// asmjit::JitRuntime runtime_;
};

/// \endcond

} // namespace geodesk
