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

    uint32_t stub() const
    {
        return static_cast<uint32_t>(guid_[0]) << 24
            | static_cast<uint32_t>(guid_[1]) << 16
            | static_cast<uint32_t>(guid_[2]) << 8
            | static_cast<uint32_t>(guid_[3]);
    }

    static UUID create();

    char* format(char* buf) const;

    template<typename Stream>
    void format(Stream& out) const
    {
        char buf[64];
        char* p = format(buf);
        out.write(buf, p - buf);
    }

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
