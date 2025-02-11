// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/util/Buffer.h>
#include <cstring>
#include <clarisma/util/log.h>

namespace clarisma {

FileBuffer::FileBuffer(FILE* file, size_t capacity) :
	file_(file)
{
	buf_ = new char[capacity];
	p_ = buf_;
	end_ = buf_ + capacity;
}


FileBuffer::~FileBuffer()
{
	if (buf_) delete[] buf_;
    fclose(file_);          // TODO: Check!!!!!
}

void FileBuffer::filled(char* p)
{
    flush(p);
}

void FileBuffer::flush(char* p)
{
    assert(p >= buf_);
    assert(p <= end_);
	assert(file_);
	//LOGS << "FileBuffer: Writing " << (p-buf_) << " bytes";
	size_t written = fwrite(buf_, 1, p - buf_, file_);
	//LOGS << "  FileBuffer: Wrote " << written << " bytes";
	// TODO: check number of bytes written (error handling)
	p_ = buf_;
}

} // namespace clarisma
