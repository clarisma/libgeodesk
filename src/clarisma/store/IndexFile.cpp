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

} // namespace clarisma
