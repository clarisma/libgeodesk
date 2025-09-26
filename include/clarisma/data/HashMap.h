// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <gtl/phmap.hpp>

namespace clarisma {

/// A faster replacement for std::unordered_map
/// Alias for GTL Flat Hash Map by Gregory Popovitch
/// https://github.com/greg7mdp/gtl, licensed under Apache 2.0

template<class K,
         class V,
         class Hash  = gtl::priv::hash_default_hash<K>,
         class Eq    = gtl::priv::hash_default_eq<K>,
         class Alloc = gtl::priv::Allocator<gtl::priv::Pair<const K, V>>> // alias for std::allocator
using HashMap = gtl::flat_hash_map<K,V,Hash,Eq,Alloc>;

} // namespace clarisma
