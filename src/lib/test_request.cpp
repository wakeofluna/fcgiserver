#include "request.h"
#include "test_mock_cgi_data.h"
#include "test_mock_logger.h"
#include <catch2/catch_test_macros.hpp>

using namespace fcgiserver;

namespace
{

const char *g_envp[] = {
    "FCGI_ROLE=RESPONDER",
    "QUERY_STRING=",
    "REQUEST_METHOD=GET",
    "CONTENT_TYPE=",
    "CONTENT_LENGTH=",
    "SCRIPT_NAME=/test",
    "REQUEST_URI=/////test",
    "DOCUMENT_URI=/test",
    "DOCUMENT_ROOT=/tmp",
    "SERVER_PROTOCOL=HTTP/1.1",
    "REQUEST_SCHEME=http",
    "GATEWAY_INTERFACE=CGI/1.1",
    "SERVER_SOFTWARE=nginx/1.21.1",
    "REMOTE_ADDR=192.168.2.7",
    "REMOTE_PORT=59542",
    "SERVER_ADDR=192.168.2.7",
    "SERVER_PORT=80",
    "SERVER_NAME=fcgiserver.lan",
    "REDIRECT_STATUS=200",
    "HTTP_HOST=fcgiserver.lan",
    "ILLEGAL_ENTRY",
    "HTTP_USER_AGENT=Mozilla/5.0 (X11; Linux x86_64; rv:88.0) Gecko/20100101 Firefox/88.0",
    "HTTP_ACCEPT=text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8",
    "HTTP_ACCEPT_LANGUAGE=en-US,en;q=0.5",
    "HTTP_ACCEPT_ENCODING=gzip, deflate",
    "HTTP_DNT=1",
    "HTTP_CONNECTION=keep-alive",
    "HTTP_UPGRADE_INSECURE_REQUESTS=1",
    "HTTP_SEC_GPC=1",
    "HTTP_CACHE_CONTROL=max-age=0",
    nullptr
};
constexpr size_t g_envp_size = (sizeof(g_envp) / sizeof(*g_envp)) - 1; // nullptr deducted

}


TEST_CASE("Request", "[request]")
{
	SECTION("EnvMap")
	{
		MockCgiData cgidata(std::string(), g_envp);
		Logger logger = MockLogger::create();
		Request request(cgidata, logger);

		auto & env_map = request.env_map();
		REQUIRE(env_map.size() == g_envp_size - 1); // illegal entry deducted

		decltype(env_map.cbegin()) iter;

		iter = env_map.find("HTTP_DNT");
		REQUIRE(iter != env_map.cend());
		REQUIRE(iter->second == "1");

		iter = env_map.find("HTTP_ACCEPT_");
		REQUIRE(iter == env_map.cend());

		iter = env_map.find("ILLEGAL_ENTRY");
		REQUIRE(iter == env_map.cend());

		iter = env_map.find("REQUEST_METHOD");
		REQUIRE(iter != env_map.cend());
		REQUIRE(iter->second == "GET");
	}
}

TEST_CASE("Request-RequestMethod", "[request]")
{
	const char *envp[] = {
	    nullptr,
	    nullptr
	};

	MockCgiData cgidata(std::string(), envp);
	Logger logger = MockLogger::create();
	Request request(cgidata, logger);

	SECTION("UNKNOWN")
	{
		envp[0] = "Request_Method=GET";
		REQUIRE(request.request_method() == RequestMethod::Unknown);
		REQUIRE(request.request_method_string().empty());
	}

	SECTION("GET")
	{
		envp[0] = "REQUEST_METHOD=GET";
		REQUIRE(request.request_method() == RequestMethod::GET);
		REQUIRE(request.request_method_string() == "GET");
	}

	SECTION("POST")
	{
		envp[0] = "REQUEST_METHOD=POST";
		REQUIRE(request.request_method() == RequestMethod::POST);
		REQUIRE(request.request_method_string() == "POST");
	}

	SECTION("PUT")
	{
		envp[0] = "REQUEST_METHOD=PUT";
		REQUIRE(request.request_method() == RequestMethod::PUT);
		REQUIRE(request.request_method_string() == "PUT");
	}

	SECTION("DELETE")
	{
		envp[0] = "REQUEST_METHOD=DELETE";
		REQUIRE(request.request_method() == RequestMethod::DELETE);
		REQUIRE(request.request_method_string() == "DELETE");
	}

	SECTION("OTHER-OTHER")
	{
		envp[0] = "REQUEST_METHOD=OTHER";
		REQUIRE(request.request_method() == RequestMethod::Other);
		REQUIRE(request.request_method_string() == "OTHER");
	}

	SECTION("OTHER-GETT")
	{
		envp[0] = "REQUEST_METHOD=GETT";
		REQUIRE(request.request_method() == RequestMethod::Other);
		REQUIRE(request.request_method_string() == "GETT");
	}
}

TEST_CASE("Request-Query", "[request]")
{
	const char *envp[] = {
	    "REQUEST_METHOD=GET",
	    nullptr,
	    nullptr
	};

	MockCgiData cgidata(std::string(), envp);
	Logger logger = MockLogger::create();
	Request request(cgidata, logger);

	SECTION("Not provided")
	{
		REQUIRE(request.query_string() == "");

		auto qmap = request.query();
		REQUIRE(qmap.empty());
	}

	SECTION("Empty")
	{
		envp[1] = "QUERY_STRING=";
		REQUIRE(request.query_string() == "");

		auto qmap = request.query();
		REQUIRE(qmap.empty());
	}

	SECTION("Single element")
	{
		envp[1] = "QUERY_STRING=foo=bar";
		REQUIRE(request.query_string() == "foo=bar");

		auto qparam = request.query();
		REQUIRE(qparam.size() == 1);

		std::pair<bool,std::string_view> iter;

		iter = request.query("foo");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "bar");
	}

	SECTION("Multiple elements")
	{
		envp[1] = "QUERY_STRING=foo=bar&test=whatever&there=no-spoon";
		REQUIRE(request.query_string() == "foo=bar&test=whatever&there=no-spoon");

		auto qparam = request.query();
		REQUIRE(qparam.size() == 3);

		std::pair<bool,std::string_view> iter;

		iter = request.query("foo");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "bar");

		iter = request.query("test");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "whatever");

		iter = request.query("there");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "no-spoon");
	}

	SECTION("Multiple elements without value")
	{
		envp[1] = "QUERY_STRING=foo=bar&test&there=no-spoon&beef";
		REQUIRE(request.query_string() == "foo=bar&test&there=no-spoon&beef");

		auto qparam = request.query();
		REQUIRE(qparam.size() == 4);

		std::pair<bool,std::string_view> iter;

		iter = request.query("foo");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "bar");

		iter = request.query("test");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "");

		iter = request.query("there");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "no-spoon");

		iter = request.query("beef");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "");
	}

	SECTION("Multiple elements with equal signs in value")
	{
		envp[1] = "QUERY_STRING=foo=bar&test&there=no-spoon=true&beef";
		REQUIRE(request.query_string() == "foo=bar&test&there=no-spoon=true&beef");

		auto qparam = request.query();
		REQUIRE(qparam.size() == 4);

		std::pair<bool,std::string_view> iter;

		iter = request.query("foo");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "bar");

		iter = request.query("test");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "");

		iter = request.query("there");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "no-spoon=true");

		iter = request.query("beef");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "");
	}

	SECTION("Elements that dont exist")
	{
		envp[1] = "QUERY_STRING=foo=bar&test&there=no-spoon&beef";
		REQUIRE(request.query_string() == "foo=bar&test&there=no-spoon&beef");

		auto qparam = request.query();
		REQUIRE(qparam.size() == 4);

		std::pair<bool,std::string_view> iter;

		iter = request.query("bar");
		REQUIRE(!iter.first);

		iter = request.query("no");
		REQUIRE(!iter.first);

		iter = request.query("spoon");
		REQUIRE(!iter.first);

		iter = request.query("bee");
		REQUIRE(!iter.first);

		iter = request.query("test&");
		REQUIRE(!iter.first);
	}

	SECTION("Duplicate elements")
	{
		envp[1] = "QUERY_STRING=foo=test1&bar=test2&foo=test3";
		REQUIRE(request.query_string() == "foo=test1&bar=test2&foo=test3");

		auto qparam = request.query();
		REQUIRE(qparam.size() == 3);

		std::pair<bool,std::string_view> iter;

		iter = request.query("foo");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "test1");

		iter = request.query("bar");
		REQUIRE(iter.first);
		REQUIRE(iter.second == "test2");

		REQUIRE(qparam[0].first == "foo");
		REQUIRE(qparam[1].first == "bar");
		REQUIRE(qparam[2].first == "foo");

		REQUIRE(qparam[0].second == "test1");
		REQUIRE(qparam[1].second == "test2");
		REQUIRE(qparam[2].second == "test3");
	}

	SECTION("Percent decoding")
	{
		std::string result;

		result = Request::query_decode("test%20environment");
		REQUIRE( result == "test environment" );

		result = Request::query_decode("percent%25hack");
		REQUIRE( result == "percent%hack" );

		result = Request::query_decode("invalid%a%Percent");
		REQUIRE( result == "invalid%a%Percent");

		result = Request::query_decode("%48%49%4A%4B%4c%4d%4E%4f%50%51");
		REQUIRE( result == "HIJKLMNOPQ" );

		result = Request::query_decode("trailing%20percent%");
		REQUIRE( result == "trailing percent%");

		result = Request::query_decode("trailing%20percent%2");
		REQUIRE( result == "trailing percent%2");

		result = Request::query_decode("trailing%20percent%20");
		REQUIRE( result == "trailing percent ");

		result = Request::query_decode("multibyte%20is%20evil%E2%84%a2right%40");
		REQUIRE( result == "multibyte is evil\xe2\x84\xa2right@");

		result = Request::query_decode("multibyte%20invalid%20evil%E2%F4%A2right%40");
		REQUIRE( result == "multibyte invalid evil\xe2\xf4\xa2right@");
	}
}

TEST_CASE("Request-Port", "[request]")
{
	const char *envp[] = {
	    nullptr,
	    nullptr
	};

	MockCgiData cgidata(std::string(), envp);
	Logger logger = MockLogger::create();
	Request request(cgidata, logger);

	SECTION("Not provided")
	{
		REQUIRE(request.remote_port_string() == "");

		int port = request.remote_port();
		REQUIRE(port == -1);
	}

	SECTION("Empty")
	{
		envp[0] = "REMOTE_PORT=";
		REQUIRE(request.remote_port_string() == "");

		int port = request.remote_port();
		REQUIRE(port == -1);
	}

	SECTION("Valid port")
	{
		envp[0] = "REMOTE_PORT=6542";
		REQUIRE(request.remote_port_string() == "6542");

		int port = request.remote_port();
		REQUIRE(port == 6542);
	}

	SECTION("Port out of range")
	{
		envp[0] = "REMOTE_PORT=66542";
		REQUIRE(request.remote_port_string() == "66542");

		int port = request.remote_port();
		REQUIRE(port == -1);
	}

	SECTION("Not a number")
	{
		envp[0] = "REMOTE_PORT=:8000";
		REQUIRE(request.remote_port_string() == ":8000");

		int port = request.remote_port();
		REQUIRE(port == -1);
	}
}

TEST_CASE("Request-DoNotTrack", "[request]")
{
	const char *envp[] = {
	    nullptr,
	    nullptr
	};

	MockCgiData cgidata(std::string(), envp);
	Logger logger = MockLogger::create();
	Request request(cgidata, logger);

	SECTION("Not provided")
	{
		REQUIRE(request.do_not_track_string() == "");

		bool dnt = request.do_not_track();
		REQUIRE(dnt == false);
	}

	SECTION("Empty")
	{
		envp[0] = "HTTP_DNT=";
		REQUIRE(request.do_not_track_string() == "");

		bool dnt = request.do_not_track();
		REQUIRE(dnt == false);
	}

	SECTION("Zero")
	{
		envp[0] = "HTTP_DNT=0";
		REQUIRE(request.do_not_track_string() == "0");

		bool dnt = request.do_not_track();
		REQUIRE(dnt == false);
	}

	SECTION("One")
	{
		envp[0] = "HTTP_DNT=1";
		REQUIRE(request.do_not_track_string() == "1");

		bool dnt = request.do_not_track();
		REQUIRE(dnt == true);
	}

	SECTION("True")
	{
		envp[0] = "HTTP_DNT=TRUE";
		REQUIRE(request.do_not_track_string() == "TRUE");

		bool dnt = request.do_not_track();
		REQUIRE(dnt == false);
	}
}

TEST_CASE("Request-UTF8", "[request]")
{
	const char *envp[] = {
	    nullptr,
	    nullptr
	};

	MockCgiData cgidata(std::string(), envp);
	Logger logger = MockLogger::create();
	Request request(cgidata, logger);

	std::string_view line = "test \xe2\x84\xa2 true"sv;
	std::u32string_view line32 = U"test ™ true"sv;
	std::string_view line_invalid = "test \xe2\xf4\xa2 true"sv;
	std::u32string_view line_invalid_expected = U"test � true"sv;

	// Force out the headers first for the writing tests
	request.send_headers();
	cgidata.m_writebuf.clear();

	SECTION("Decode UTF8")
	{
		auto result = request.utf8_decode(line);
		REQUIRE( result == line32 );
	}

	SECTION("Decode invalid UTF8")
	{
		auto result = request.utf8_decode(line_invalid);
		REQUIRE( result == line_invalid_expected );
	}

	SECTION("Encode UTF8")
	{
		auto result = request.utf8_encode(line32);
		REQUIRE( result == line );
	}

	SECTION("Write UTF8 as HTML")
	{
		request.set_encoding(ContentEncoding::HTML);
		request.write_stream() << line;
		REQUIRE( cgidata.m_writebuf == "test &#x2122; true" );
	}

	SECTION("Write invalid UTF8 as HTML")
	{
		request.set_encoding(ContentEncoding::HTML);
		request.write_stream() << line_invalid;
		REQUIRE( cgidata.m_writebuf == "test &#xfffd; true" );
	}

	SECTION("Write UTF32 as HTML")
	{
		request.set_encoding(ContentEncoding::HTML);
		request.write_stream() << line32;
		REQUIRE( cgidata.m_writebuf == "test &#x2122; true" );
	}

	SECTION("Write UTF8 as UTF8")
	{
		request.set_encoding(ContentEncoding::UTF8);
		request.write_stream() << line;
		REQUIRE( cgidata.m_writebuf == line );
	}

	SECTION("Write UTF32 as UTF8")
	{
		request.set_encoding(ContentEncoding::UTF8);
		request.write_stream() << line32;
		REQUIRE( cgidata.m_writebuf == line );
	}

	SECTION("Error UTF8 as UTF8")
	{
		request.set_encoding(ContentEncoding::HTML);
		request.error_stream() << line;
		REQUIRE( cgidata.m_errorbuf == line );
	}

	SECTION("Error UTF32 as UTF8")
	{
		request.set_encoding(ContentEncoding::HTML);
		request.error_stream() << line32;
		REQUIRE( cgidata.m_errorbuf == line );
	}
}

TEST_CASE("Request-Route", "[request]")
{
	const char *envp[] = {
	    nullptr,
	    nullptr
	};

	MockCgiData cgidata(std::string(), envp);
	Logger logger = MockLogger::create();
	Request request(cgidata, logger);

	SECTION("No URI")
	{
		auto route = request.full_route();
		REQUIRE( route.empty() );
	}

	SECTION("Root URI")
	{
		envp[0] = "DOCUMENT_URI=/";
		auto route = request.full_route();
		REQUIRE( route.empty() );
	}

	SECTION("URI with one path component no trailing slash")
	{
		envp[0] = "DOCUMENT_URI=/foobar";
		auto route = request.full_route();
		REQUIRE( route.size() == 1 );
		REQUIRE( route[0] == "foobar"sv );
	}

	SECTION("URI with one path component and trailing slash")
	{
		envp[0] = "DOCUMENT_URI=/foobar/";
		auto route = request.full_route();
		REQUIRE( route.size() == 1 );
		REQUIRE( route[0] == "foobar"sv );
	}

	SECTION("URI with three components")
	{
		envp[0] = "DOCUMENT_URI=/foobar/dead/beef/";
		auto route = request.full_route();
		REQUIRE( route.size() == 3 );
		REQUIRE( route[0] == "foobar"sv );
		REQUIRE( route[1] == "dead"sv );
		REQUIRE( route[2] == "beef"sv );
	}
}
