#ifndef TEST_MOCK_LOGGER_H
#define TEST_MOCK_LOGGER_H

#include "i_log_callback.h"
#include "logger.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

class MockLogger : public fcgiserver::ILogCallback
{
public:
	MockLogger();
	~MockLogger();

	static fcgiserver::Logger create();

	void log_message(fcgiserver::LogLevel level, std::string_view const& message) override;
	using fcgiserver::ILogCallback::now;

	std::vector<std::string> log_debug;
	std::vector<std::string> log_info;
	std::vector<std::string> log_error;
};

#endif // TEST_MOCK_LOGGER_H
