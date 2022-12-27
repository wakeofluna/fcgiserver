#include "line_formatter.h"
#include "symbols.h"
#include <catch2/catch_test_macros.hpp>
#include <climits>

using namespace fcgiserver;

TEST_CASE("LineFormatter", "[logger]")
{
	LineFormatter lf;

	SECTION("Initially empty and space reserved")
	{
		REQUIRE( lf.empty() );
		REQUIRE( lf.buffer().capacity() > 100 );

		const size_t capacity = lf.buffer().capacity();
		const char * data = lf.buffer().c_str();

		lf.append("abcdefghijklmnopqrstvuwxyz");

		REQUIRE( !lf.empty() );
		REQUIRE( lf.buffer().capacity() == capacity );
		REQUIRE( lf.buffer().c_str() == data );
	}

	SECTION("All sorts of operators")
	{
		lf << 'a';
		REQUIRE( lf.buffer() == "a" );
		lf << uint8_t('a');
		REQUIRE( lf.buffer() == "a97" );
		lf << "  ";
		REQUIRE( lf.buffer() == "a97  " );
		lf << size_t(999);
		REQUIRE( lf.buffer() == "a97  999" );
		lf << std::string(8, '?');
		REQUIRE( lf.buffer() == "a97  999????????" );
		lf << 12;
		REQUIRE( lf.buffer() == "a97  999????????12" );
		lf << 5.2f;
		REQUIRE( lf.buffer() == "a97  999????????125.2" );
		lf << 12345678.333;
		REQUIRE( lf.buffer() == "a97  999????????125.212345678.333" );

		lf.clear();

		REQUIRE( lf.empty() );
		REQUIRE( lf.buffer() == "" );

		lf.append(uint16_t(65535), int16_t(-18));
		REQUIRE( lf.buffer() == "65535-18" );

		lf << true << false << symbols::CONTENT_TYPE;
		REQUIRE( lf.buffer() == "65535-18truefalseCONTENT_TYPE" );
	}

	SECTION("printf")
	{
		lf.printf("A %s and a %d", "string", 43);
		REQUIRE( lf.buffer() == "A string and a 43" );

		lf.clear();

		lf.printf("Something hex 0x%x and padded 0x%08x", 0xbeef, 0xf00d);
		REQUIRE( lf.buffer() == "Something hex 0xbeef and padded 0x0000f00d" );

		lf.clear();

		std::string expected = "A really really " + std::string(200, '.') + " really long string";
		lf.printf("A really really %s really long string", std::string(200, '.').c_str());
		REQUIRE( lf.buffer() == expected );
	}

	SECTION("Going long")
	{
		const size_t capacity = lf.buffer().capacity();
		const char * data = lf.buffer().c_str();

		lf.append(std::string(capacity - 10, '-'));

		REQUIRE( lf.buffer().capacity() == capacity );
		REQUIRE( lf.buffer().c_str() == data );

		lf.append(std::string(20, '_'));

		REQUIRE( lf.buffer().capacity() >= capacity + 10 );
		REQUIRE( lf.buffer().c_str() != data );
	}
}
