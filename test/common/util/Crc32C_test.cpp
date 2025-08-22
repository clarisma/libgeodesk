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
