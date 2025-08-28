// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/libero/FreeStore.h>
#include <clarisma/util/Crc32C.h>


namespace clarisma {

class FreeStore::Journal
{
public:
	FreeStore::Journal() {}

	void open(const char* fileName, uint32_t bufSize = 64 * 1024);
	void close();
	void addBlock(uint64_t marker, const void* content, size_t size);
	void reset(uint64_t marker, const Header* header);
	void seal();

private:
	void computeChecksum();
	void writeToFile();

	File file_;
	std::unique_ptr<byte[]> buf_;
	byte* end_ = nullptr;
	uint64_t filePos_ = 0;
	uint32_t bufPos_ = 0;
	Crc32C checksum_;
};

} // namespace clarisma
