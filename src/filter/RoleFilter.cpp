// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/filter/RoleFilter.h>
#include <clarisma/data/SmallVector.h>

namespace geodesk {

using namespace clarisma;

RoleFilter::RoleFilter(std::span<std::string_view> roles, const StringTable& strings) :
    Filter(ROLE_FILTER, FeatureTypes::RELATION_MEMBERS)
{
    SmallVector<int,8> codes;
    size_t size = 0;
    constexpr size_t MAX_ROLE_LEN = 255;

    for (auto role : roles)
    {
        int code = strings.getCode(role);
        codes.push_back(code);
        size_t strLen = std::min(role.size(), MAX_ROLE_LEN);
        size += code < 0 ? (strLen + 1) : 2;
        globalRoleCount_ += code >= 0;
        localRoleCount_ += code < 0;
    }

    if (size > sizeof(inlineData_))    [[unlikely]]
    {
        data_ = new uint8_t[size];
    }

    uint16_t* pCode = reinterpret_cast<uint16_t*>(data_);
    uint8_t* pStr = data_ + sizeof(uint16_t) * globalRoleCount_;

    for (int i = 0; i<codes.size(); i++)
    {
        int code = codes[i];
        if (code >= 0)
        {
            assert(code < (1 << 16));
            *pCode++ = static_cast<uint16_t>(code);
        }
        else
        {
            std::string_view role = roles[i];
            size_t strLen = std::min(role.size(), MAX_ROLE_LEN);
            *pStr++ = static_cast<uint8_t>(strLen);
            memcpy(pStr, role.data(), strLen);
            pStr += strLen;
        }
    }
    assert(pStr == data_ + size);
}

bool RoleFilter::acceptRole(int roleCode, const ShortVarString* roleStr) const
{
    const uint8_t* p = data_;
    if (roleCode >= 0)  [[likely]]
    {
        assert(roleCode < (1 << 16));
        const uint16_t* pCode = reinterpret_cast<const uint16_t*>(p);
        for (int i=0; i<globalRoleCount_; i++)
        {
            if (static_cast<uint16_t>(roleCode) == *pCode++) return true;
        }
    }
    else
    {
        p += sizeof(uint16_t) * globalRoleCount_;
        for (int i=0; i<localRoleCount_; i++)
        {
            int len = *p++;
            if (*roleStr == std::string_view(reinterpret_cast<const char*>(p), len))
            {
                return true;
            }
            p += len;
        }
    }
    return false;
}

} // namespace geodesk
