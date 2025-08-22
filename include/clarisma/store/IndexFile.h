// Copyright (c) 2023 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cassert>
#include <clarisma/io/ExpandableMappedFile.h>

namespace clarisma {

/**
 * A fast disk-based key-value lookup that stores values in an array,
 * appropriate for keyspaces that are expected to be dense
 * 
 * - Values can be comprised of 1 to 24 bits and are stored in 
 *   packed form -- bits() must be called prior to use
 * - Care must be taken to use the same bit size for the same file
 *   (The bit size is not stored in the file)
 * - Values are stored within 4-KB blocks in a way that an access
 *   never reads across a block boundary (both for performance 
 *   reasons and to avoid segfaults)
 * - The file is sparse; blocks whose key range is unused don't
 *   take up disk space
 * - The file expands as needed, following the rules of the
 *   ExpandableMappedFile class
 */

class IndexFile : protected ExpandableMappedFile
{
public:
	IndexFile();

	void open(const char* filename, OpenMode mode, int valueWidth);


	uint32_t get(uint64_t key);
	void put(uint64_t key, uint32_t value);

	void clear() { ExpandableMappedFile::clear(); }

private:
	struct CellRef
	{
		uint64_t* p;
		int bitOffset;
	};

	CellRef getCell(int64_t key);

    int64_t slotsPerSegment_;
		// has to be 64 bit to support 1-bit indexes
		// could avoid by limiting to 4B slots per segment, for 1-bit indexes
		// each segment will only be half-filled, but that's ok since index files are sparse
    int64_t maxEntryCount_;
	int valueWidth_;
};

} // namespace clarisma
