// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdint>
#include <boost/crc.hpp>

namespace clarisma {

// TODO: Switch to CRC32C (hardware support, greater strength)

class Crc32
{
public:
    void update(const void* data, size_t size)
    {
        crc_.process_bytes(data, size);
    }

    uint32_t get() const noexcept { return crc_.checksum(); }

private:
    boost::crc_32_type crc_;
};


} // namespace clarisma
