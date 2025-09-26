// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/math/Decimal.h>
#include <geodesk/feature/TagTablePtr.h>

namespace geodesk {

/// \cond lowlevel

// Alternative to TagIterator

class TagWalker
{
public:
	TagWalker(TagTablePtr tags, StringTable& strings) :
		tags_(tags),
		p_(tags_.ptr()),
		strings_(strings)
	{
	}

	bool next()
	{
		if (p_.ptr() < tags_.ptr())	[[unlikely]]
		{
			if (!p_) [[likely]] return false;
			int64_t tag = p_.getLongUnaligned();
			int32_t rawPointer = static_cast<int32_t>(tag >> 16);
			int32_t flags = rawPointer & 7;
			// local keys are relative to the 4-byte-aligned tagtable address
			key_ = reinterpret_cast<const clarisma::ShortVarString*>(
				tags_.alignedBasePtr().ptr() + ((rawPointer ^ flags) >> 1));
			keyCode_ = -1;
			value_ = (static_cast<TagBits>(tags_.pointerOffset(p_) - 2) << 32)
                | ((tag & 0xffff) << 16) | flags;
			p_ = (flags & 4) ? DataPtr() : (p_ - 6 - (flags & 2));
			return true;
		}
		// TODO: maybe just fetch key/value separately
		//  to avoid unaligned read issue altogether
		uint32_t tag = p_.getUnsignedIntUnaligned();
		keyCode_ = (tag & 0x7fff) >> 2;
		key_ = strings_.getGlobalString(keyCode_);
		value_ = (static_cast<TagBits>(tags_.pointerOffset(p_) + 2) << 32) | tag;
		int lastFlag = tag & 0x8000;
		// If we're at the end:
		//   Move pointer to first local tag (if any), or null
		// else:
		//   Move pointer to the next tag (bit 1 indicates whether value is
		//   2 or 4 bytes wide)
		p_ = lastFlag ? (tags_.hasLocalKeys() ? (tags_.ptr() - 6) : DataPtr()) : (p_ + 4 + (tag & 2));
		return true;
	}

	TagTablePtr tags() const { return tags_; }
	StringTable& strings() const { return strings_; }
    int keyCode() const { return keyCode_; }
	const clarisma::ShortVarString* key() const { return key_; }
	TagBits rawValue() const { return value_; };
	TagValueType valueType() { return static_cast<TagValueType>(value_ & 3); }
    TagValue value() const { return tags_.tagValue(value_, strings_); }
    bool isStringValue() const { return value_ & 1; }
	bool isNumericValue() const { return !isStringValue(); }
	bool isWideValue() const { return value_ & 2; }

	const clarisma::ShortVarString* stringValueFast() const
    {
    	if(isWideValue())	[[unlikely]]
        {
        	return localStringValueFast();
        }
        return globalStringValueFast();
    }

	const clarisma::ShortVarString* globalStringValueFast() const
	{
		return tags_.globalString(value_, strings_);
	}

	const clarisma::ShortVarString* localStringValueFast() const
	{
		return tags_.localString(value_);
	}

	clarisma::Decimal numberValueFast() const
    {
		if(isWideValue())	[[unlikely]]
		{
			return tags_.wideNumber(value_);
        }
		return clarisma::Decimal(TagTablePtr::narrowNumber(value_), 0);
	}

private:
	TagTablePtr tags_;
	DataPtr p_;
    int keyCode_;
    const clarisma::ShortVarString* key_;
    TagBits value_;
	StringTable& strings_;
};

// \endcond
} // namespace geodesk
