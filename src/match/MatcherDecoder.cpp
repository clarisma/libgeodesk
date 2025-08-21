// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include "MatcherDecoder.h"
#include <clarisma/util/ShortVarString.h>
#include "MatcherEmitter.h"			// TODO: refactor
#include "OpGraph.h"
#include <geodesk/feature/FeatureStore.h>

namespace geodesk {

using namespace clarisma;

void MatcherDecoder::decode()
{
	const uint16_t* p = pCodeStart_;
	for (;;)
	{
		int opcode = *p & 0xff;
		int flags = (*p >> 8);
		switch (opcode)
		{
		// non-branching ops
		case Opcode::NOP:
		case Opcode::CODE_TO_STR:
			writeOpcodeStub(p);
			out_.writeByte('\n');
			break;

		case Opcode::GOTO:
		{
			writeOpcodeStub(p++);
			out_.writeByte(' ');
			const uint16_t* pTarget = p + static_cast<int16_t>(*p) / 2;		// target offset uses bytes, not words
			if (pTarget > pLastInstruction_) pLastInstruction_ = pTarget;
			writeAddress(pTarget, false);
			out_.writeByte('\n');
			p++;
			if (p > pLastInstruction_)
			{
				out_.flush();
				return;
			}
			continue;
		}

		case Opcode::RETURN:
			writeOpcodeStub(p);
			out_ << (flags ? " TRUE\n" : " FALSE\n");
			p++;
			if (p > pLastInstruction_)
			{
				out_.flush();
				return;
			}
			continue;

		default:
			writeBranchingOp(p);
			break;
		}
		p += OPCODE_ARGS[opcode] + 1;
	}
}


void MatcherDecoder::writeAddress(const uint16_t* p, bool padded)
{
	char buf[32];
	Format::unsafe(buf, padded ? "%5d" : "%d", (p - pCodeStart_));
	out_ << buf;
}

void MatcherDecoder::writeOpcodeStub(const uint16_t* p)
{
	writeAddress(p, true);
	out_ << "  " << OPCODE_NAMES[*p & 0xff];
}

void MatcherDecoder::writeBranchingOp(const uint16_t* p)
{
	int opcode = *p & 0xff;
	bool negated = (*p >> 8) & 1;
	writeAddress(p, true);
	out_ << (negated ? "  NOT " : "  ") << OPCODE_NAMES[*p & 0xff];
	p++;
	switch (OPCODE_OPERAND_TYPES[opcode])
	{
		case OperandType::CODE:
		{
			uint16_t code = *p++;
			const ShortVarString* str = store_->strings().getGlobalString(code);
			out_ << ' ' << str << " (" << code << ')';
		}
		break;

		case OperandType::STRING:
		{
			uint16_t ofs = *p;
			const StringResource* pStr = reinterpret_cast<const StringResource*>(
				reinterpret_cast<const uint8_t*>(p) - ofs);
			out_ << " \"";
			out_.write(pStr->data, pStr->len);
			out_.writeByte('\"');
			p++;
		}
		break;

		case OperandType::DOUBLE:
		{
			uint16_t ofs = *p;
			const double* pDouble = reinterpret_cast<const double*>(
				reinterpret_cast<const uint8_t*>(p) - ofs);
			out_ << " " << *pDouble;
			p++;
		}
		break;

		case OperandType::REGEX:
		{
			uint16_t ofs = *p;
			const std::regex* pRegex = reinterpret_cast<const std::regex*>(
				reinterpret_cast<const uint8_t*>(p) - ofs);
			out_ << " <regex>";
			p++;
		}
		break;

		case OperandType::FEATURE_TYPES:
		{
			out_ << " <TODO>";
			p += 2;	// two-word argument
		}
		break;

		default:
			// no argument
			break;
	}
	const uint16_t* pTarget = p + static_cast<int16_t>(*p) / 2;		// target offset uses bytes, not words
	if (pTarget > pLastInstruction_) pLastInstruction_ = pTarget;

	out_ << " -> ";
	writeAddress(pTarget, false);
	out_.writeByte('\n');
}


} // namespace geodesk
