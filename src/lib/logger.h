#ifndef FCGISERVER_LOGGER_H
#define FCGISERVER_LOGGER_H

#include <memory>
#include "fcgiserver_defs.h"
#include "i_log_callback.h"
#include "line_formatter.h"

namespace fcgiserver
{

class LogStream;
class LoggerPrivate;
class DLL_PUBLIC Logger
{
public:
	Logger();
	Logger(std::unique_ptr<ILogCallback> && callback);
	Logger(Logger const& other) = delete;
	Logger(Logger && other) = delete;
	Logger& operator= (Logger const& other) = delete;
	Logger& operator= (Logger && other) = delete;
	~Logger();

	void set_log_callback(std::unique_ptr<ILogCallback> && callback);
	ILogCallback * log_callback() const;

	void log(LogLevel level, std::string_view const& message) const;
	void logf(LogLevel level, const char *fmt, ...) const;

	LogStream operator<< (LogLevel level) const;
	inline LogStream stream(LogLevel level) const;
	inline LogStream debug() const;
	inline LogStream info() const;
	inline LogStream error() const;

private:
	LoggerPrivate * m_private;
};

class DLL_PUBLIC LogStream : public LineFormatter
{
public:
	LogStream(Logger const& logger, LogLevel level);
	~LogStream();

private:
	Logger const& m_logger;
	LogLevel m_level;
};

inline LogStream Logger::stream(LogLevel level) const { return operator<< (level); }
inline LogStream Logger::debug() const { return operator<< (LogLevel::Debug); }
inline LogStream Logger::info() const { return operator<< (LogLevel::Info); }
inline LogStream Logger::error() const { return operator<< (LogLevel::Error); }

}

#endif // FCGISERVER_LOGGER_H
