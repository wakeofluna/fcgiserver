#include "console_log_callback.h"
#include <chrono>
#include <cstdio>
#include <mutex>

using namespace fcgiserver;

ConsoleLogCallback::ConsoleLogCallback() = default;

ConsoleLogCallback::~ConsoleLogCallback() = default;

void ConsoleLogCallback::log_message(LogLevel level, std::string_view const& message)
{
	static std::mutex mutex;

	std::lock_guard<std::mutex> guard(mutex);

	switch (level)
	{
		case fcgiserver::LogLevel::Debug:
			std::fwrite("[DEBUG] ", 1, 8, stdout);
			std::fwrite(message.begin(), 1, message.size(), stdout);
			std::fputc('\n', stdout);
			std::fflush(stdout);
			break;
		case fcgiserver::LogLevel::Info:
			std::fwrite("[INFO] ", 1, 7, stdout);
			std::fwrite(message.begin(), 1, message.size(), stdout);
			std::fputc('\n', stdout);
			std::fflush(stdout);
			break;
		case fcgiserver::LogLevel::Error:
			std::fwrite("[ERROR] ", 1, 8, stderr);
			std::fwrite(message.begin(), 1, message.size(), stderr);
			std::fputc('\n', stderr);
			std::fflush(stderr);
			break;
	}
}
