// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdio>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <clarisma/alloc/Block.h>
#include <clarisma/text/Format.h>

namespace clarisma {

class Buffer
{
public:
	Buffer() : buf_(nullptr) {}
	virtual ~Buffer() {} // TODO: make noexcept

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
			if (len <= remainingCapacity)
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

	/*
	Buffer& operator<<(std::string_view s)
	{
		write(s);
		return *this;
	}

	Buffer& operator<<(const char* s)
	{
		write(s, strlen(s));
		return *this;
	}

	Buffer& operator<<(char ch)
	{
		writeByte(ch);
		return *this;
	}

	Buffer& operator<<(int64_t n)
	{
		char buf[32];
		char* end = buf + sizeof(buf);
		char* start = Format::integerReverse(n, end);
		write(start, end - start);
		return *this;
	}

	Buffer& operator<<(uint64_t n)
	{
		char buf[32];
		char* end = buf + sizeof(buf);
		char* start = Format::unsignedIntegerReverse(n, end);
		write(start, end - start);
		return *this;
	}

	Buffer& operator<<(double d)
	{
		char buf[64];
		char* end = buf + sizeof(buf);
		char* start = Format::doubleReverse(&end, d);
		write(start, end - start);
		return *this;
	}
	*/

	operator std::string_view() const
	{
		return std::string_view(buf_, length());
	}

	void clear()
	{
		p_ = buf_;
	}

protected:
	// TODO: Only works if len will fit into a flushed/resized buffer
	void ensureCapacityUnsafe(size_t len)
	{
		if (capacityRemaining() < len) filled(p_);
		assert(capacityRemaining() >= len);
	}

	void putStringUnsafe(const char* s, size_t len)
	{
		assert(capacityRemaining() >= len);
		memcpy(p_, s, len);
		p_ += len;
	}

	template <size_t N>
	void putStringUnsafe(const char(&s)[N])
	{
		putStringUnsafe(s, N-1);	// Subtract 1 to exclude null terminator
	}

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


// TODO: replace with FileBuffer2
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

template<typename T>
concept BufferLike = std::is_base_of_v<Buffer, T>;

template<BufferLike B>
B& operator<<(B& buf, std::string_view s)
{
	buf.write(s);
	return buf;
}

template<BufferLike B>
B& operator<<(B& buf, const char* s)
{
	buf.write(s, std::strlen(s));
	return buf;
}

template<BufferLike B>
B& operator<<(B& buf, char ch)
{
	buf.writeByte(ch);
	return buf;
}

template<BufferLike B>
B& operator<<(B& buf, int64_t n)
{
	char tmp[32];
	char* end = tmp + sizeof(tmp);
	char* start = Format::integerReverse(n, end);
	buf.write(start, end - start);
	return buf;
}

template<BufferLike B>
B& operator<<(B& buf, int n)
{
	buf << static_cast<int64_t>(n);
	return buf;
}

template<BufferLike B>
B& operator<<(B& buf, uint64_t n)
{
	char tmp[32];
	char* end = tmp + sizeof(tmp);
	char* start = Format::unsignedIntegerReverse(n, end);
	buf.write(start, end - start);
	return buf;
}

template<BufferLike B>
B& operator<<(B& buf, double d)
{
	char tmp[64];
	char* end = tmp + sizeof(tmp);
	char* start = Format::doubleReverse(&end, d);
	buf.write(start, end - start);
	return buf;
}

} // namespace clarisma
