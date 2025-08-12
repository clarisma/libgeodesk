// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/io/File.h>
#include <clarisma/util/Buffer.h>

namespace clarisma {

class FileBuffer2 : public Buffer
{
public:
	explicit FileBuffer2(size_t capacity = 64 * 1024)
	{
		buf_ = new char[capacity];
		p_ = buf_;
		end_ = buf_ + capacity;
	}

	~FileBuffer2() override
	{
		if (p_ > buf_) file_.write(buf_, p_ - buf_);
		delete[] buf_;
	}

	void open(const char* filename, File::OpenMode mode =
		File::OpenMode::CREATE | File::OpenMode::WRITE | File::OpenMode::REPLACE_EXISTING)
	{
		file_.open(filename, mode);
	}

	void close()
	{
		file_.close();
	}

	void filled(char* p) override
	{
		flush(p);
	}

	void flush(char* p) override
	{
		file_.write(buf_, p - buf_);
		p_ = buf_;
	}

	void flush()
	{
		flush(p_);
	}

private:
	File file_;
};


} // namespace clarisma
