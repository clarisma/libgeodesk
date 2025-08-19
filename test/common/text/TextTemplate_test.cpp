// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include "clarisma/text/TextTemplate.h"
#include "clarisma/util/DynamicStackBuffer.h"

using namespace clarisma;

TEST_CASE("Basic TextTemplate")
{
	TextTemplate::Ptr t = TextTemplate::compile("Hello {fname}!");
	DynamicStackBuffer<1024> s;

	t->write(s,
		[](std::string_view k) -> std::string_view
		{
			if (k == "fname") return "George";
			return {};
		});

	REQUIRE(static_cast<std::string_view>(s) == "Hello George!");

	s.clear();
	t->write(s,
		[](std::string_view k) -> std::string_view
		{
			return {};
		});
	REQUIRE(static_cast<std::string_view>(s) == "Hello !");

	s.clear();
	TextTemplate::Ptr t2 = TextTemplate::compile("{monkey  }{ \trabbit  }");
	t2->write(s,
		[](std::string_view k) -> std::string_view
		{
			if (k == "monkey") return "banana";
			if (k == "rabbit") return "carrot";
			return {};
		});
	REQUIRE(static_cast<std::string_view>(s) == "bananacarrot");

}
