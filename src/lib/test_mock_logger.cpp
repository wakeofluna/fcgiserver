#include "test_mock_logger.h"
#include "test_mock_cgi_data.h"
#include "request.h"

using namespace fcgiserver;

MockLogger::MockLogger() = default;

MockLogger::~MockLogger() = default;

Logger MockLogger::create()
{
	return Logger(std::make_unique<MockLogger>());
}

void MockLogger::log_message(LogLevel level, std::string_view const& message)
{
	switch (level)
	{
		case LogLevel::Debug:
			log_debug.push_back(std::string(message));
			break;
		case LogLevel::Info:
			log_info.push_back(std::string(message));
			break;
		case LogLevel::Error:
			log_error.push_back(std::string(message));
			break;
	}
}


#include <catch2/catch_test_macros.hpp>
#include <ctime>

TEST_CASE("MockLogger", "[logger]")
{
	Logger logger = MockLogger::create();
	MockLogger * mock_logger = static_cast<MockLogger*>(logger.log_callback());

	SECTION("Request logging has a sane default")
	{
		const char *envp[] = {
		    "QUERY_STRING=foo=bar",
		    "REQUEST_METHOD=DELETE",
		    "CONTENT_TYPE=plain/text",
		    "CONTENT_LENGTH=109",
		    "SCRIPT_NAME=/script_name",
		    "REQUEST_URI=/////request_uri",
		    "DOCUMENT_URI=/document_uri",
		    "REQUEST_SCHEME=http",
		    "REMOTE_ADDR=192.168.2.7",
		    "REMOTE_PORT=59542",
		    nullptr,
		};

		MockCgiData cgidata(std::string(), envp);
		Request request(cgidata);

		request.set_http_status(887);
		logger.log_callback()->log_request(request);

		REQUIRE( mock_logger->log_debug.size() == 1 );
		std::string entry = mock_logger->log_debug.front();

		REQUIRE( entry.find("887") != std::string::npos );
		REQUIRE( entry.find("192.168.2.7") != std::string::npos );
		REQUIRE( entry.find("59542") != std::string::npos );
		REQUIRE( entry.find("DELETE") != std::string::npos );
		REQUIRE( entry.find("document_uri") != std::string::npos );
	}

	SECTION("now() function returns localtime")
	{
		time_t tt_c;
		struct tm tm_c;
		std::time(&tt_c);
		localtime_r(&tt_c, &tm_c);

		struct tm tm_m;
		MockLogger::now(&tm_m);

		// I'll take the risk that it is not within one second ...
		REQUIRE( tm_c.tm_year == tm_m.tm_year );
		REQUIRE( tm_c.tm_mon  == tm_m.tm_mon );
		REQUIRE( tm_c.tm_mday == tm_m.tm_mday );
		REQUIRE( tm_c.tm_hour == tm_m.tm_hour );
		REQUIRE( tm_c.tm_min  == tm_m.tm_min );
		REQUIRE( tm_c.tm_sec  == tm_m.tm_sec );
	}
}
