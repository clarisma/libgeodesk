// Copyright (c) 2023 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cassert>
#include <clarisma/io/ExpandableMappedFile.h>

#include "clarisma/util/DataPtr.h"

namespace clarisma {

/**
 * A fast disk-based key-value lookup that stores values in an array,
 * appropriate for keyspaces that are expected to be dense
 * 
 * - Values can be comprised of 1 to 24 bits and are stored in 
 *   packed form -- bits() must be called prior to use
 * - Care must be taken to use the same bit size for the same file
 *   (The bit size is not stored in the file)
 * - Values are stored within 1-GB segments in a way that an access
 *   never reads across a segment boundary
 *   TODO: consider confining values to 4KB pages?
 * - The file is sparse; blocks whose key range is unused don't
 *   take up disk space
 * - The file expands as needed, following the rules of the
 *   ExpandableMappedFile class
 */

class IndexFile : protected ExpandableMappedFile
{
public:
	/**
	 * Sequentially iterates the values of an IndexFile, starting
	 * at key 0. Each call to next() returns the value of the next key.
	 *
	 * Iteration covers only the keys that lie within the current
	 * extent of the file; the iterator never causes the file to grow.
	 * Once all keys have been visited, next() keeps returning 0
	 * (consistent with the value of any key that has never been set).
	 *
	 * Values are read a 64-bit word at a time and buffered, so each
	 * word of the index is loaded from memory only once.
	 *
	 * The iterator becomes invalid if the IndexFile is closed or
	 * cleared. Because of buffering, a value written via put() during
	 * iteration may not be seen even if its key has not been visited
	 * yet (if its word has already been loaded); keys beyond the
	 * extent captured by iter() are not visited.
	 */
	class Iterator2
	{
	public:
		uint32_t next();

		/// @brief Returns true if there are keys left to visit
		///
		[[nodiscard]] bool hasNext() const { return key_ < endKey_; }

		/// @brief The key whose value will be returned by the
		/// next call to next()
		///
		[[nodiscard]] uint64_t key() const { return key_; }

	private:
		Iterator2(IndexFile& file, uint64_t endKey);
		void startSegment();

		IndexFile& file_;
		const uint64_t* p_;     // next word to load into the buffer
		uint64_t buf_;          // unconsumed bits, LSB-aligned; bits
		//  above bitsInBuf_ are always zero
		uint64_t key_;
		uint64_t endKey_;
		uint64_t remainingInSegment_;
		uint64_t mask_;
		int bitsInBuf_;
		int valueWidth_;

		friend class IndexFile;
	};

	class Iterator
	{
	public:
		uint32_t next();

	private:
		Iterator(IndexFile& file);
		void startSegment();

		uint64_t buf_;
		uint32_t mask_;
		int bitOfs_;
		int valueWidth_;
		int segment_ = 0;
		DataPtr p_;
		const uint8_t* end_;
		IndexFile& file_;

		friend class IndexFile;
	};

	IndexFile();

	void open(const char* filename, OpenMode mode, int valueWidth);

	uint32_t get(uint64_t key);
	void put(uint64_t key, uint32_t value);

	Iterator iter();

	void clear() { ExpandableMappedFile::clear(); }

private:
	struct CellRef
	{
		uint64_t* p;
		int bitOffset;
	};

	CellRef getCell(int64_t key);

	uint64_t valueMask() const
	{
		return (uint64_t{1} << valueWidth_) - 1;
	}

    int64_t slotsPerSegment_;
		// has to be 64 bit to support 1-bit indexes
		// could avoid by limiting to 4B slots per segment, for 1-bit indexes
		// each segment will only be half-filled, but that's ok since index files are sparse
    int64_t maxEntryCount_;
	int valueWidth_;
};

} // namespace clarisma
