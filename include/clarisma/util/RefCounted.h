// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#ifdef GEODESK_MULTITHREADED
#include <atomic>
#else
#include <cstdint>
#endif

namespace clarisma {

class RefCounted
{
public:
	RefCounted() : refcount_(1) {}
	virtual ~RefCounted() {};   // needs to have a virtual destructor

#ifdef GEODESK_MULTITHREADED
	void addref() const
	{
		refcount_.fetch_add(1, std::memory_order_relaxed);
	}

	void release() const
	{
		if (refcount_.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			delete this;
		}
	}
#else
	void addref() const { ++refcount_; }
	void release() const
	{
		if (--refcount_ == 0)
		{
			delete this;
		}
	}
#endif

private:
#ifdef GEODESK_MULTITHREADED
	mutable std::atomic_uint32_t refcount_;
#else
	mutable uint32_t refcount_;
#endif
};

} // namespace clarisma
