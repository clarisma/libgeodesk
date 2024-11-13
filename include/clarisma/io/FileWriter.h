// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/util/StreamWriter.h>
#include <filesystem>
#include <clarisma/io/FileBuffer2.h>

namespace clarisma {

class FileWriter : public AbstractStreamWriter<FileWriter>
{
public:
	FileWriter()
	{
		setBuffer(&buf_);
	}

	explicit FileWriter(const char* filename, size_t capacity = 64 * 1024) :
		buf_(capacity)
	{
		setBuffer(&buf_);
		open(filename);
	}

	explicit FileWriter(std::filesystem::path path, size_t capacity = 64 * 1024) :
		buf_(capacity)
	{
		setBuffer(&buf_);
		open(path);
	}

	~FileWriter()
	{
		flush();		// TODO: should base class always flush?
	}

	void open(const char* filename)
	{
		setBuffer(&buf_);
		buf_.open(filename);
	}

	void open(std::filesystem::path path)
	{
		buf_.open(path.string().c_str());
	}

private:
	FileBuffer2 buf_;
};

} // namespace clarisma
