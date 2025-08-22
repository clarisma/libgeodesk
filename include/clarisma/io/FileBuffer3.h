// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/io/FileHandle.h>
#include <clarisma/util/Buffer.h>

namespace clarisma {

class FileBuffer3 : public Buffer
{
public:
	explicit FileBuffer3(size_t capacity = 64 * 1024)
	{
		buf_ = new char[capacity];
		p_ = buf_;
		end_ = buf_ + capacity;
		ownBuffer_ = true;
		ownFile_ = false;
	}

	explicit FileBuffer3(std::unique_ptr<char[]>&& buf, size_t size)
	{
		buf_ = buf.release();
		p_ = buf_;
		end_ = buf_ + size;
		ownBuffer_ = true;
		ownFile_ = false;
	}

	explicit FileBuffer3(char* buf, size_t size)
	{
		buf_ = buf;
		p_ = buf_;
		end_ = buf_ + size;
		ownBuffer_ = false;
		ownFile_ = false;
	}

	explicit FileBuffer3(FileHandle file, size_t capacity = 64 * 1024)
	{
		buf_ = new char[capacity];
		p_ = buf_;
		end_ = buf_ + capacity;
		file_ = file;
		ownBuffer_ = true;
		ownFile_ = false;
	}

	FileBuffer3(FileHandle file, std::unique_ptr<char[]>&& buf, size_t size)
	{
		buf_ = buf.release();
		p_ = buf_;
		end_ = buf_ + size;
		file_ = file;
		ownBuffer_ = true;
		ownFile_ = false;
	}

	FileBuffer3(FileHandle file, char* buf, size_t size)
	{
		buf_ = buf;
		p_ = buf_;
		end_ = buf_ + size;
		file_ = file;
		ownBuffer_ = false;
		ownFile_ = false;
	}

	~FileBuffer3() noexcept override
	{
		if (file_.isOpen())
		{
			if (p_ > buf_)
			{
				size_t bytesWritten;
				file_.tryWrite(buf_, p_ - buf_, bytesWritten);
			}
			if (ownFile_) file_.tryClose();
		}
		if (ownBuffer_) delete[] buf_;
	}

	FileHandle fileHandle() const { return file_; }

	void open(const char* filename, File::OpenMode mode =
		File::OpenMode::CREATE | File::OpenMode::WRITE | File::OpenMode::REPLACE_EXISTING)
	{
		close();
		file_.open(filename, mode);
		ownFile_ = true;
	}

	void close()
	{
		if (ownFile_) file_.close();
	}

	void filled(char* p) override
	{
		flush(p);
	}

	void flush(char* p) override
	{
		file_.writeAll(buf_, p - buf_);
		p_ = buf_;
	}

	void flush()
	{
		flush(p_);
	}

private:
	FileHandle file_;
	bool ownBuffer_;
	bool ownFile_;
};


} // namespace clarisma
