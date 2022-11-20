#include "symbol.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

using namespace fcgiserver;

TEST_CASE("Symbols", "[symbol]")
{
	SECTION("Symbols from cstrings")
	{
		Symbol s1 = "asdf";
		Symbol s2("qwer");
		Symbol s3;
		s3 = "fudge";

		REQUIRE( s1 != s2 );
		REQUIRE( s1 != s3 );
		REQUIRE( s2 != s3 );

		std::string_view v1 = s1.to_string_view();
		std::string_view v2 = s2.to_string_view();
		std::string_view v3 = s3.to_string_view();

		REQUIRE( v1.size() == 4 );
		REQUIRE( v2.size() == 4 );
		REQUIRE( v3.size() == 5 );

		REQUIRE( std::memcmp(v1.data(), "asdf", 5) == 0 );
		REQUIRE( std::memcmp(v2.data(), "qwer", 5) == 0 );
		REQUIRE( std::memcmp(v3.data(), "fudge", 6) == 0 );
	}

	SECTION("Symbols from string_view")
	{
		char const line[] = "In the age of chaos two factions battled for dominance";

		std::string_view l1(line, 10);
		std::string_view l2(line + 14, 5);
		std::string_view l3(line + 37, 11);

		Symbol s1 = l1;
		Symbol s2(l2);
		Symbol s3;
		s3 = l3;

		REQUIRE( s1 != s2 );
		REQUIRE( s1 != s3 );
		REQUIRE( s2 != s3 );

		std::string_view v1 = s1.to_string_view();
		std::string_view v2 = s2.to_string_view();
		std::string_view v3 = s3.to_string_view();

		REQUIRE( v1.size() == 10 );
		REQUIRE( v2.size() == 5 );
		REQUIRE( v3.size() == 11 );

		REQUIRE( std::memcmp(v1.data(), "In the age", 11) == 0 );
		REQUIRE( std::memcmp(v2.data(), "chaos", 6) == 0 );
		REQUIRE( std::memcmp(v3.data(), "led for dom", 12) == 0 );
	}

	SECTION("Symbol comparisons")
	{
		char const line[] = "aabbbbcc";
		Symbol s1("bbbb");
		Symbol s2(std::string_view(line+2, 4));

		REQUIRE( s1 == s2 );

		Symbol s3("d");
		Symbol s4("a");

		REQUIRE( s1 < s3 );
		REQUIRE( s4 < s2 );
		REQUIRE( s4 < s3 );
	}
}
