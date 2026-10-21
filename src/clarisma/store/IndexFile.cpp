// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/store/IndexFile.h>
#include <limits>

namespace clarisma {

IndexFile::IndexFile() :
	slotsPerSegment_(0),
	maxEntryCount_(std::numeric_limits<int64_t>::max()),	// TODO
	valueWidth_(0)
{
}

void IndexFile::open(const char* filename, OpenMode mode, int valueWidth)
{
	ExpandableMappedFile::open(filename, mode);
	assert(valueWidth > 0 && valueWidth <= 32);
	valueWidth_ = valueWidth;
	slotsPerSegment_ = static_cast<int64_t>(SEGMENT_LENGTH) * 8 / valueWidth_;
}

IndexFile::CellRef IndexFile::getCell(int64_t key)
{
	lldiv_t d = lldiv(key, slotsPerSegment_);
	int64_t segmentNo = d.quot;
	int64_t slot = d.rem;
	d = lldiv(slot * valueWidth_, 8 * sizeof(uint64_t));
	CellRef ref;
	ref.p = reinterpret_cast<uint64_t*>(
		translate(static_cast<uint64_t>(SEGMENT_LENGTH)
			* segmentNo + d.quot * sizeof(uint64_t)));
	ref.bitOffset = static_cast<int>(d.rem);
	return ref;
}

// TODO: Clarify that it is legal to call get() if IndexFile is not open,
//  which will always return 0 because maxEntryCount_==0
uint32_t IndexFile::get(uint64_t key)
{
	if (key >= maxEntryCount_) [[unlikely]]
	{
		return 0;
	}
	assert(isOpen());
	CellRef ref = getCell(key);
	uint64_t mask = (1 << valueWidth_) - 1;
	uint64_t v = *ref.p >> ref.bitOffset;
	int overflow = ref.bitOffset + valueWidth_ - 64;
	if (overflow > 0)
	{
		v |= *(ref.p + 1) << (valueWidth_ - overflow);
	}
	return static_cast<int>(v & mask);
}

void IndexFile::put(uint64_t key, uint32_t value)
{
	assert(isOpen());
	if (key >= maxEntryCount_) [[unlikely]]
	{
		return;
	}

	CellRef ref = getCell(key);
	uint64_t mask = (1 << valueWidth_) - 1;
	assert((value & mask) == value);	// value must fit in range

	uint64_t* p = ref.p;
	*p = *p & ~(mask << ref.bitOffset)
		| (static_cast<uint64_t>(value) << ref.bitOffset);

	int overflow = ref.bitOffset + valueWidth_ - 64;
	if (overflow > 0)
	{
		p++;
		int shift = valueWidth_ - overflow;
		*p = *p	& ~(mask >> shift)
			| (static_cast<uint64_t>(value) >> shift);
	}
}


IndexFile::Iterator IndexFile::iter()
{
    uint64_t endKey = 0;
    if (isOpen() && valueWidth_ > 0)
    {
        uint64_t fileSize = size();
        uint64_t fullSegments = fileSize / SEGMENT_LENGTH;
        uint64_t partialBytes = fileSize % SEGMENT_LENGTH;
        // Only count slots whose bits lie entirely within the file
        uint64_t partialSlots = std::min<uint64_t>(
            partialBytes * 8 / valueWidth_,
            static_cast<uint64_t>(slotsPerSegment_));
        endKey = fullSegments * static_cast<uint64_t>(slotsPerSegment_)
            + partialSlots;
        endKey = std::min<uint64_t>(endKey,
            static_cast<uint64_t>(maxEntryCount_));
    }
    return {*this};
}

IndexFile::Iterator2::Iterator2(IndexFile& file, uint64_t endKey) :
    file_(file),
    p_(nullptr),
    buf_(0),
    key_(0),
    endKey_(endKey),
    remainingInSegment_(0),
    mask_(endKey ? file.valueMask() : 0),
    bitsInBuf_(0),
    valueWidth_(file.valueWidth_)
{
}

void IndexFile::Iterator2::startSegment()
{
    // key_ is always at the start of a segment when we get here
    uint64_t slotsPerSegment = static_cast<uint64_t>(file_.slotsPerSegment_);
    assert(key_ % slotsPerSegment == 0);
    uint64_t segmentNo = key_ / slotsPerSegment;
    p_ = reinterpret_cast<const uint64_t*>(
        file_.translate(segmentNo * SEGMENT_LENGTH));
    // Discard any padding bits left over from the previous segment
    buf_ = 0;
    bitsInBuf_ = 0;
    remainingInSegment_ = std::min(slotsPerSegment, endKey_ - key_);
}

uint32_t IndexFile::Iterator2::next()
{
    if (remainingInSegment_ == 0) [[unlikely]]
    {
        if (key_ >= endKey_) return 0;
        startSegment();
    }

    uint64_t v;
    if (bitsInBuf_ >= valueWidth_) [[likely]]
    {
        // Common case: value is entirely in the buffer
        v = buf_;
        buf_ >>= valueWidth_;
        bitsInBuf_ -= valueWidth_;
    }
    else
    {
        // Combine the remaining low bits of the value (if any) with
        // the next word. The word is guaranteed to lie within the
        // segment, since it holds at least one bit of this value.
        // Shifts are safe: bitsInBuf_ is in [0, valueWidth_), so
        // the right shift is in [1, valueWidth_]
        uint64_t word = *p_++;
        v = buf_ | (word << bitsInBuf_);
        int consumed = valueWidth_ - bitsInBuf_;
        buf_ = word >> consumed;
        bitsInBuf_ = 64 - consumed;
    }
    remainingInSegment_--;
    key_++;
    return static_cast<uint32_t>(v & mask_);
}

IndexFile::Iterator::Iterator(IndexFile& file) :
	mask_(file.valueMask()),
	valueWidth_(file.valueWidth_),
	file_(file)
{
	startSegment();
}


void IndexFile::Iterator::startSegment()
{
	p_ = DataPtr(file_.translate(segment_ * SEGMENT_LENGTH));
	end_ = p_.ptr() + SEGMENT_LENGTH - 4;
	buf_ = p_.getUnsignedLong();
	p_ += 4;
	bitOfs_ = 0;
}

uint32_t IndexFile::Iterator::next()
{
	int nextBitOfs = bitOfs_ + valueWidth_;
	if (nextBitOfs > 64) [[unlikely]]
	{
		if (p_.ptr() == end_) [[unlikely]]
		{
			segment_++;
			startSegment();
		}
		else
		{
			buf_ = p_.getUnsignedLong();
			p_ += 4;
			bitOfs_ -= 32;
			nextBitOfs -= 32;
		}
	}
	uint32_t value = static_cast<uint32_t>((buf_ >> bitOfs_) & mask_);
	bitOfs_ = nextBitOfs;
	return value;
}

} // namespace clarisma
