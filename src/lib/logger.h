#ifndef FCGISERVER_LOGGER_H
#define FCGISERVER_LOGGER_H

#include <cstdint>
#include <functional>

namespace fcgiserver
{

enum class LogLevel : std::uint8_t
{
	Debug,
	Info,
	Error,
};

using LogCallback = std::function<void(LogLevel, const char *)>;

}

#endif // FCGISERVER_LOGGER_H
