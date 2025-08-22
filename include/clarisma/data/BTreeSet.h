// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <gtl/btree.hpp>

namespace clarisma {

/// Alias for GTL btree_Set by Gregory Popovitch
/// https://github.com/greg7mdp/gtl, licensed under Apache 2.0

template<class K,
         class Compare = gtl::Less<K>,
         class Alloc   = gtl::priv::Allocator<K>> // alias for std::allocator
using BTreeSet = gtl::btree_set<K,Compare,Alloc>;

} // namespace clarisma
