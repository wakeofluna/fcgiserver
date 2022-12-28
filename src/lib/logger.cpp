#include "logger.h"
#include <chrono>
#include <cstring>

using namespace fcgiserver;


class fcgiserver::LoggerPrivate
{
public:
	std::unique_ptr<ILogCallback> callback;
};


Logger::Logger()
    : m_private(new LoggerPrivate)
{
}

Logger::Logger(std::unique_ptr<ILogCallback> && callback)
    : m_private(new LoggerPrivate)
{
	m_private->callback = std::move(callback);
}

Logger::~Logger()
{
	delete m_private;
}

void Logger::set_log_callback(std::unique_ptr<ILogCallback> && callback)
{
	m_private->callback = std::move(callback);
}

ILogCallback * Logger::log_callback() const
{
	return m_private->callback.get();
}

void Logger::log(LogLevel level, std::string_view const& message) const
{
	if (!m_private->callback)
		return;

	size_t start = 0;
	while (start < message.size())
	{
		// Split by newline
		auto newline = message.find('\n', start);
		if (newline == std::string::npos)
			newline = message.size();

		// Trim right
		size_t end = newline;
		while (end > start && message[end-1] <= 32)
			--end;

		// Detect empty lines
		if (start < end)
		{
			std::string_view submessage = message.substr(start, end - start);
			m_private->callback->log_message(level, submessage);
		}

		start = newline + 1;
	}
}

void Logger::logf(LogLevel level, const char *fmt, ...) const
{
	std::va_list vl;
	va_start(vl, fmt);
	stream(level).vprintf(fmt, vl);
	va_end(vl);
}

LogStream Logger::operator<< (LogLevel level) const
{
	return LogStream(*this, level);
}

LogStream::LogStream(Logger const& log, LogLevel lvl)
    : LineFormatter(GenericFormat::UTF8)
    , m_logger(log)
    , m_level(lvl)
{
}

LogStream::~LogStream()
{
	m_logger.log(m_level, m_buffer);
}
