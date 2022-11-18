#include "request.h"
#include "test_mock_cgi_data.h"
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
		Request request(cgidata);

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
	Request request(cgidata);

	SECTION("UNKNOWN")
	{
		envp[0] = "Request_Method=GET";
		REQUIRE(request.request_method() == RequestMethod::UNKNOWN);
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
		REQUIRE(request.request_method() == RequestMethod::OTHER);
		REQUIRE(request.request_method_string() == "OTHER");
	}

	SECTION("OTHER-GETT")
	{
		envp[0] = "REQUEST_METHOD=GETT";
		REQUIRE(request.request_method() == RequestMethod::OTHER);
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
	Request request(cgidata);

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

		auto qmap = request.query();
		REQUIRE(qmap.size() == 1);

		decltype(qmap.cbegin()) iter;

		iter = qmap.find("foo");
		REQUIRE(iter != qmap.end());
		REQUIRE(iter->second == "bar");
	}

	SECTION("Multiple elements")
	{
		envp[1] = "QUERY_STRING=foo=bar&test=whatever&there=no-spoon";
		REQUIRE(request.query_string() == "foo=bar&test=whatever&there=no-spoon");

		auto qmap = request.query();
		REQUIRE(qmap.size() == 3);

		decltype(qmap.cbegin()) iter;

		iter = qmap.find("foo");
		REQUIRE(iter != qmap.end());
		REQUIRE(iter->second == "bar");

		iter = qmap.find("test");
		REQUIRE(iter != qmap.end());
		REQUIRE(iter->second == "whatever");

		iter = qmap.find("there");
		REQUIRE(iter != qmap.end());
		REQUIRE(iter->second == "no-spoon");
	}

	SECTION("Multiple elements without value")
	{
		envp[1] = "QUERY_STRING=foo=bar&test&there=no-spoon&beef";
		REQUIRE(request.query_string() == "foo=bar&test&there=no-spoon&beef");

		auto qmap = request.query();
		REQUIRE(qmap.size() == 4);

		decltype(qmap.cbegin()) iter;

		iter = qmap.find("foo");
		REQUIRE(iter != qmap.end());
		REQUIRE(iter->second == "bar");

		iter = qmap.find("test");
		REQUIRE(iter != qmap.end());
		REQUIRE(iter->second == "");

		iter = qmap.find("there");
		REQUIRE(iter != qmap.end());
		REQUIRE(iter->second == "no-spoon");

		iter = qmap.find("beef");
		REQUIRE(iter != qmap.end());
		REQUIRE(iter->second == "");
	}

	SECTION("Multiple elements with equal signs in value")
	{
		envp[1] = "QUERY_STRING=foo=bar&test&there=no-spoon=true&beef";
		REQUIRE(request.query_string() == "foo=bar&test&there=no-spoon=true&beef");

		auto qmap = request.query();
		REQUIRE(qmap.size() == 4);

		decltype(qmap.cbegin()) iter;

		iter = qmap.find("foo");
		REQUIRE(iter != qmap.end());
		REQUIRE(iter->second == "bar");

		iter = qmap.find("test");
		REQUIRE(iter != qmap.end());
		REQUIRE(iter->second == "");

		iter = qmap.find("there");
		REQUIRE(iter != qmap.end());
		REQUIRE(iter->second == "no-spoon=true");

		iter = qmap.find("beef");
		REQUIRE(iter != qmap.end());
		REQUIRE(iter->second == "");
	}
}

TEST_CASE("Request-Port", "[request]")
{
	const char *envp[] = {
	    nullptr,
	    nullptr
	};

	MockCgiData cgidata(std::string(), envp);
	Request request(cgidata);

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
	Request request(cgidata);


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
