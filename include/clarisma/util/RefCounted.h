// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <stdint.h>

namespace clarisma {

class RefCounted
{
public:
	RefCounted() : refcount_(1) {}
	virtual ~RefCounted() {};   // needs to have a virtual destructor

	void addref() const { ++refcount_; }
	void release() const
	{
		if (--refcount_ == 0)
		{
			delete this;
		}
	}

private:
	mutable uint32_t refcount_;
};

} // namespace clarisma
