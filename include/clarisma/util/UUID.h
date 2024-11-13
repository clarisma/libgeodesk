// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace clarisma {

using std::byte;

class UUID
{
public:
    UUID()  // NOLINT initialization in body
    {
        memset(guid_, 0, sizeof(guid_));
    }

    explicit UUID(const uint8_t* bytes)    // NOLINT initialization in body
    {
        memcpy(guid_, bytes, sizeof(guid_));
    }

    explicit UUID(const UUID& other)       // NOLINT initialization in body
    {
        memcpy(guid_, other.guid_, sizeof(guid_));
    }

    bool operator==(const UUID& other) const
    {
        return memcmp(guid_, other.guid_, sizeof(guid_)) == 0;
    }

    bool operator!=(const UUID& other) const
    {
        return !(*this == other);
    }

    static UUID create();

    char* format(char* buf) const;

private:
    uint8_t guid_[16];
};

template<typename Stream>
Stream& operator<<(Stream& out, const UUID& uuid)
{
    char buf[64];
    char* p = uuid.format(buf);
    out.write(buf, p - buf);
    return out;
}


} // namespace clarisma
