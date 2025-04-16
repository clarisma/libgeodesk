// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdio>
#include <cstring>
#include <string_view>
#include <clarisma/alloc/Block.h>

namespace clarisma {

class Buffer
{
public:
	Buffer() : buf_(nullptr) {}
	virtual ~Buffer() {}

	const char* data() const { return buf_; }
	char* pos() const { return p_; }
	char* start() const { return buf_; }
	char* end() const { return end_; }
	size_t length() const { return p_ - buf_; }
	size_t capacity() const { return end_ - buf_; }
	bool isEmpty() const { return p_ == buf_; }

	size_t capacityRemaining() const
	{
		return end_ - p_;
	}

	virtual void filled(char *p) = 0;
	virtual void flush(char* p) = 0;

	void write(const void* data, size_t len)
	{
		const char* b = reinterpret_cast<const char*>(data);
		for (;;)
		{
			size_t remainingCapacity = capacityRemaining();
			if (len < remainingCapacity)
			{
				std::memcpy(p_, b, len);
				p_ += len;
				return;
			}
			std::memcpy(p_, b, remainingCapacity);
			p_ += remainingCapacity;
			filled(p_);
			b += remainingCapacity;
			len -= remainingCapacity;
		}
	}

	void write(std::string_view s)
	{
		write(s.data(), s.size());
	}

	void writeByte(int ch)
	{
		*p_++ = ch;
		if (p_ == end_) filled(p_);
	}

	Buffer& operator<<(std::string_view s)
	{
		write(s);
		return *this;
	}

	operator std::string_view() const
	{
		return std::string_view(buf_, length());
	}

protected:
	char* buf_;
	char* p_;
	char* end_;
};


class DynamicBuffer : public Buffer
{
public:
	explicit DynamicBuffer(size_t initialCapacity);
	~DynamicBuffer() override;

	DynamicBuffer(DynamicBuffer&& other) noexcept
	{
		buf_ = other.buf_;
		p_ = other.p_;
		end_ = other.end_;
		other.buf_ = nullptr;
		other.p_ = nullptr;
		other.end_ = nullptr;
	}

	DynamicBuffer& operator=(DynamicBuffer&& other) noexcept
	{
		if (this != &other)
		{
			if(buf_) delete[] buf_;
			buf_ = other.buf_;
			p_ = other.p_;
			end_ = other.end_;
			other.buf_ = nullptr;
			other.p_ = nullptr;
			other.end_ = nullptr;
		}
		return *this;
	}

	// Disable copy constructor and copy assignment operator
	DynamicBuffer(const DynamicBuffer&) = delete;
	DynamicBuffer& operator=(const DynamicBuffer&) = delete;

	void filled(char* p) override;		// TODO: why does this take a pointer??
	void flush(char* p) override;
	ByteBlock takeBytes();

protected:
	void grow();
};

/*
class FrugalBuffer : public DynamicBuffer
{
public:
	FrugalBuffer(size_t initialHeapCapacity);
	virtual ~DynamicBuffer() {};

	virtual void flush();

protected:
	void grow();
};
*/

class FileBuffer : public Buffer
{
public:
	FileBuffer(FILE* file, size_t capacity);
	virtual ~FileBuffer();
	
	virtual void filled(char* p);
	virtual void flush(char* p);

private:
	FILE* file_;
};

} // namespace clarisma
