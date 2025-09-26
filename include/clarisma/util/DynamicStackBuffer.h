// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/util/Buffer.h>

namespace clarisma {

template<size_t N>
class DynamicStackBuffer : public Buffer
{
public:
	DynamicStackBuffer()
    {
		buf_ = stackBuf;
		p_ = buf_;
		end_ = buf_ + N;
    }

	~DynamicStackBuffer() noexcept override
    {
		if (buf_ != stackBuf) delete[] buf_;
    }

	// Disable move/copy constructor/assignment-operator
	DynamicStackBuffer(const DynamicStackBuffer&) = delete;
	DynamicStackBuffer& operator=(const DynamicStackBuffer&) = delete;
	DynamicStackBuffer(DynamicStackBuffer&&) = delete;
	DynamicStackBuffer& operator=(DynamicStackBuffer&&) = delete;

	void filled(char* p) override
	{
		assert(p >= buf_);
		assert(p <= end_);
		p_ = p;
		size_t newCapacity = (end_ - buf_) * 2;
		size_t len = length();
		char* newBuf = new char[newCapacity];
		memcpy(newBuf, buf_, len);
		p_ = newBuf + len;
		if(buf_ != stackBuf) delete[] buf_;
		buf_ = newBuf;
		end_ = newBuf + newCapacity;
	}

    void flush(char* p) override
	{
		assert(p >= buf_);
		assert(p <= end_);
		p_ = p;
	}

private:
    char stackBuf[N];
};

} // namespace clarisma
