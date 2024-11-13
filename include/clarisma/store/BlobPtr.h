// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/store/BlobStore.h>
#include <clarisma/util/DataPtr.h>

namespace clarisma {

// TODO: Changes in 2.0

class BlobPtr
{
public:
	BlobPtr(const uint8_t* p) : p_(p) {}

	static uint32_t headerSize() { return 8; }

	uint32_t payloadSize() const
	{
		const BlobStore::Blob* blob = reinterpret_cast<const BlobStore::Blob*>(p_.ptr());
		return blob->payloadSize;
	}

	uint32_t totalSize() const
	{
		return payloadSize() + headerSize();
	}

	DataPtr ptr() const { return p_; }

protected:
	DataPtr p_;
};

} // namespace clarisma
