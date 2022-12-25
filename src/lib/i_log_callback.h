#ifndef FCGISERVER_ILOGCALLBACK_H
#define FCGISERVER_ILOGCALLBACK_H

#include <string_view>
#include <ctime>
#include "fcgiserver_defs.h"

namespace fcgiserver
{

enum class LogLevel : std::uint8_t
{
	Debug,
	Info,
	Error,
};

class Request;

class DLL_PUBLIC ILogCallback
{
public:
	virtual ~ILogCallback();
	virtual void log_message(LogLevel level, std::string_view const& message) = 0;
	virtual void log_request(Request const& request);

protected:
	// Helper functions
	static void now(std::tm *tm);
};

}

#endif // FCGISERVER_ILOGCALLBACK_H
