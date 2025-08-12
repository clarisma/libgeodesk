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
