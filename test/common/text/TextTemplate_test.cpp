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
		[](Buffer& out, std::string_view k)
		{
			if (k == "fname") out.write("George");
		});

	REQUIRE(static_cast<std::string_view>(s) == "Hello George!");

	s.clear();
	t->write(s,
		[](Buffer& out, std::string_view k)
		{
		});
	REQUIRE(static_cast<std::string_view>(s) == "Hello !");

	s.clear();
	TextTemplate::Ptr t2 = TextTemplate::compile("{monkey  }{ \trabbit  }");
	t2->write(s,
		[](Buffer& out, std::string_view k)
		{
			std::string_view s;
			if (k == "monkey")
			{
				s = "banana";
			}
			else if (k == "rabbit")
			{
				s = "carrot";
			}
			else
			{
				return;
			}
			out.write(s);
		});
	REQUIRE(static_cast<std::string_view>(s) == "bananacarrot");

}
