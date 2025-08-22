// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdio>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <clarisma/alloc/Block.h>
// #include <clarisma/math/Decimal.h>
#include <clarisma/text/Format.h>

namespace clarisma {

// TODO: Clarify when buffer will be flushed
//  Currently, will flush if write op fills it completely,
//  so we always leave room for 1 byte

class Buffer
{
public:
	Buffer() :
		buf_(nullptr),
		p_(nullptr),
		end_(nullptr) {}

	virtual ~Buffer() noexcept {}

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
	void flush() { flush(p_); }

	// We never leave a buffer fully filled p_ == end_,
	// to guarantee that at least one byte will fit

	void write(const void* data, size_t len)
	{
		const char* b = reinterpret_cast<const char*>(data);
		for (;;)
		{
			size_t remainingCapacity = capacityRemaining();
			if (len < remainingCapacity)	// Don't use <=, see note above
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
		// We never leave a full buffer at method end
	}

	// We never leave a buffer fully filled p_ == end_,
	// to guarantee that at least one byte will fit

	void writeRepeatedChar(int ch, size_t times)
	{
		for (;;)
		{
			size_t remainingCapacity = capacityRemaining();
			if (times < remainingCapacity)		// don't use <=
			{
				std::memset(p_, ch, times);
				p_ += times;
				return;
			}
			std::memset(p_, ch, remainingCapacity);
			p_ += remainingCapacity;
			filled(p_);
			times -= remainingCapacity;
		}
	}

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
		// We never fully fill a buffer
		if (capacityRemaining() < len) filled(p_);
		assert(capacityRemaining() > len);
	}

	void putStringUnsafe(const char* s, size_t len)
	{
		assert(capacityRemaining() > len);
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
	~DynamicBuffer() noexcept override;

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

inline Buffer& operator<<(Buffer& buf, std::string_view s)
{
	buf.write(s);
	return buf;
}

inline Buffer& operator<<(Buffer& buf, const char* s)
{
	buf.write(s, std::strlen(s));
	return buf;
}

inline Buffer& operator<<(Buffer& buf, char ch)
{
	buf.writeByte(ch);
	return buf;
}

inline Buffer& operator<<(Buffer& buf, int64_t n)
{
	char tmp[32];
	char* end = tmp + sizeof(tmp);
	char* start = Format::integerReverse(n, end);
	buf.write(start, end - start);
	return buf;
}

inline Buffer& operator<<(Buffer& buf, int n)
{
	buf << static_cast<int64_t>(n);
	return buf;
}

inline Buffer& operator<<(Buffer& buf, uint64_t n)
{
	char tmp[32];
	char* end = tmp + sizeof(tmp);
	char* start = Format::unsignedIntegerReverse(n, end);
	buf.write(start, end - start);
	return buf;
}

inline Buffer& operator<<(Buffer& buf, uint32_t n)
{
	buf << static_cast<uint64_t>(n);
	return buf;
}

inline Buffer& operator<<(Buffer& buf, double d)
{
	char tmp[64];
	char* end = tmp + sizeof(tmp);
	char* start = Format::doubleReverse(&end, d);
	buf.write(start, end - start);
	return buf;
}

/*
inline Buffer& operator<<(Buffer& buf, Decimal d)
{
	d.format(buf);
	return buf;
}
*/

template<typename T>
concept BufferFormattable =
	requires (const T& t, Buffer& b)
{
	{ t.template format<Buffer>(b) } -> std::same_as<void>;
};

template<typename T> requires BufferFormattable<T>
Buffer& operator<<(Buffer& buf, const T& value)
{
	value.template format<Buffer>(buf);
	return buf;
}

template<typename B, typename T>
B& operator<<(B& b, const T& value)
	requires std::is_base_of_v<Buffer, std::remove_reference_t<B>> &&
		 !std::is_same_v<std::remove_reference_t<B>, Buffer> &&
		 BufferFormattable<T>
{
	// Call the Buffer& core (no payload template here), then return B&.
	static_cast<Buffer&>(b) << value;
	return b;
}

} // namespace clarisma
