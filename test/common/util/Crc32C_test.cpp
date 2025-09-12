// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <catch2/catch_test_macros.hpp>
#include <clarisma/util/Crc32C.h>

using namespace clarisma;


TEST_CASE("CRC32C")
{
	Crc32C crc;
	crc.update("123456789", 9);
	REQUIRE(crc.get() == 0xE3069283u);
}

namespace clarisma {
bool testCrc32C(const void* p, size_t len)
{
	constexpr uint32_t SEED = 0;  // any seed, as long as it's the same

	// Option A: compare finalized values (simple)
	return Crc32C::finalize(Crc32C::x86Raw(p, len, SEED)) ==
		   Crc32C::finalize(Crc32C::softRaw(p, len, SEED));

	// Option B: compare raw states via the proven mapping
	// return Crc32C::softRaw(p, len, SEED) ==
	//        ~Crc32C::x86Raw(p, len, ~SEED);
}
}

TEST_CASE("CRC32C: Hardware/software forms equivalent")
{
	REQUIRE(testCrc32C("123456789", 9));
}

