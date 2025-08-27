// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <type_traits>

#define CLARISMA_ENUM_FLAGS(EnumType)                                        \
constexpr EnumType operator|(EnumType a, EnumType b) noexcept              \
{                                                                          \
using U = std::underlying_type_t<EnumType>;                            \
return static_cast<EnumType>(static_cast<U>(a) | static_cast<U>(b));   \
}                                                                          \
constexpr EnumType operator&(EnumType a, EnumType b) noexcept              \
{                                                                          \
using U = std::underlying_type_t<EnumType>;                            \
return static_cast<EnumType>(static_cast<U>(a) & static_cast<U>(b));   \
}                                                                          \
constexpr EnumType& operator|=(EnumType& a, EnumType b) noexcept           \
{                                                                          \
a = a | b;                                                             \
return a;                                                              \
}                                                                          \
constexpr EnumType& operator&=(EnumType& a, EnumType b) noexcept           \
{                                                                          \
a = a & b;                                                             \
return a;                                                              \
}                                                                          \
constexpr EnumType operator~(EnumType a) noexcept           \
{                                                                          \
    return static_cast<EnumType>(~static_cast<uint64_t>(a));         \
}                                                                       \
constexpr auto has(EnumType a, EnumType b) noexcept                       \
{                                                                          \
return static_cast<bool>(a & b);                                          \
}                                                                         \
constexpr auto hasAny(EnumType a, EnumType b) noexcept              \
{                                                                          \
    return static_cast<bool>(a & b);                                          \
}                                                                         \
constexpr auto hasAll(EnumType a, EnumType b) noexcept              \
{                                                                          \
    return (a & b) == b;                                          \
}                                                                         \


/*
constexpr bool operator bool(EnumType e) noexcept                        \
{                                                                        \
    return static_cast<uint64_t>(e) != 0;                               \
}
*/
