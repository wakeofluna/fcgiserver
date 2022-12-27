#include "logger.h"
#include "test_mock_logger.h"
#include <catch2/catch_test_macros.hpp>

using namespace fcgiserver;

TEST_CASE("Logger", "[logger]")
{
	Logger logger = MockLogger::create();
	MockLogger * mock_logger = static_cast<MockLogger*>(logger.log_callback());

	SECTION("Various loglevels using log")
	{
		logger.log(LogLevel::Debug, "This is a debug message");
		REQUIRE( mock_logger->log_debug == std::vector<std::string>{ "This is a debug message" } );

		logger.log(LogLevel::Error, "This is an error message");
		REQUIRE( mock_logger->log_error == std::vector<std::string>{ "This is an error message" } );

		logger.log(LogLevel::Info, "This is an info message");
		REQUIRE( mock_logger->log_info == std::vector<std::string>{ "This is an info message" } );
	}

	SECTION("Various loglevels using logf")
	{
		logger.logf(LogLevel::Debug, "This is a debug message");
		REQUIRE( mock_logger->log_debug == std::vector<std::string>{ "This is a debug message" } );

		logger.logf(LogLevel::Error, "This is an error message");
		REQUIRE( mock_logger->log_error == std::vector<std::string>{ "This is an error message" } );

		logger.logf(LogLevel::Info, "This is an info message");
		REQUIRE( mock_logger->log_info == std::vector<std::string>{ "This is an info message" } );
	}

	SECTION("Various loglevels using named streams")
	{
		logger.debug() << "This is a debug message";
		REQUIRE( mock_logger->log_debug == std::vector<std::string>{ "This is a debug message" } );

		logger.error() << "This is an error message";
		REQUIRE( mock_logger->log_error == std::vector<std::string>{ "This is an error message" } );

		logger.info() << "This is an info message";
		REQUIRE( mock_logger->log_info == std::vector<std::string>{ "This is an info message" } );
	}

	SECTION("Various loglevels using parameterised streams")
	{
		logger.stream(LogLevel::Debug) << "This is a debug message";
		REQUIRE( mock_logger->log_debug == std::vector<std::string>{ "This is a debug message" } );

		logger.stream(LogLevel::Error) << "This is an error message";
		REQUIRE( mock_logger->log_error == std::vector<std::string>{ "This is an error message" } );

		logger.stream(LogLevel::Info) << "This is an info message";
		REQUIRE( mock_logger->log_info == std::vector<std::string>{ "This is an info message" } );
	}

	SECTION("Various loglevels using streamed streams")
	{
		logger << LogLevel::Debug << "This is a debug message";
		REQUIRE( mock_logger->log_debug == std::vector<std::string>{ "This is a debug message" } );

		logger << LogLevel::Error << "This is an error message";
		REQUIRE( mock_logger->log_error == std::vector<std::string>{ "This is an error message" } );

		logger << LogLevel::Info << "This is an info message";
		REQUIRE( mock_logger->log_info == std::vector<std::string>{ "This is an info message" } );
	}

	SECTION("logf formatting")
	{
		logger.logf(LogLevel::Debug, "A %s and a %d", "string", 43);
		REQUIRE( mock_logger->log_debug == std::vector<std::string>{ "A string and a 43" } );

		logger.logf(LogLevel::Info, "Something hex 0x%x and padded 0x%08x", 0xbeef, 0xf00d);
		REQUIRE( mock_logger->log_info == std::vector<std::string>{ "Something hex 0xbeef and padded 0x0000f00d" } );

		std::string expected = "A really really " + std::string(200, '.') + " really long string";
		logger.logf(LogLevel::Error, "A really really %s really long string", std::string(200, '.').c_str());
		REQUIRE( mock_logger->log_error == std::vector<std::string>{ expected } );
	}

	SECTION("Newline splitting")
	{
		logger.log(LogLevel::Info, "");
		REQUIRE( mock_logger->log_info.empty() );

		logger.log(LogLevel::Info, "Line 1");
		REQUIRE( mock_logger->log_info == std::vector<std::string>{ "Line 1"} );

		logger.logf(LogLevel::Info, "\nLine 2\n%sLine 3%sLine 4\n\n", "    \n", "\n");
		REQUIRE( mock_logger->log_info == std::vector<std::string>{ "Line 1", "Line 2", "Line 3", "Line 4" } );

		logger << LogLevel::Info << '\n' << 'x' << "\n\n" << "Line 6\n\n\n";
		REQUIRE( mock_logger->log_info == std::vector<std::string>{ "Line 1", "Line 2", "Line 3", "Line 4", "x", "Line 6" } );
	}

	SECTION("Honouring changing the callback")
	{
		auto new_mock_logger_ptr = std::make_unique<MockLogger>();
		auto new_mock_logger = new_mock_logger_ptr.get();

		REQUIRE( new_mock_logger != logger.log_callback() );
		logger.set_log_callback(std::move(new_mock_logger_ptr));
		REQUIRE( new_mock_logger == logger.log_callback() );

		logger.log(LogLevel::Info, "New logger test");
		REQUIRE( new_mock_logger->log_info == std::vector<std::string>{ "New logger test"} );
	}

	SECTION("Dont crash when there is no callback")
	{
		std::unique_ptr<MockLogger> no_mock_logger;
		logger.set_log_callback(std::move(no_mock_logger));
		logger.log(LogLevel::Info, "No logger test");
		// Nothing to check
	}
}
