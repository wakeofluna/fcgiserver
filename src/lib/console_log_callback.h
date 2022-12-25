#ifndef FCGISERVER_CONSOLELOGCALLBACK_H
#define FCGISERVER_CONSOLELOGCALLBACK_H

#include "fcgiserver_defs.h"
#include "i_log_callback.h"

namespace fcgiserver
{

class DLL_PRIVATE ConsoleLogCallback : public ILogCallback
{
public:
	ConsoleLogCallback();
	~ConsoleLogCallback();

	void log_message(LogLevel level, const std::string_view & message) override;
};

}

#endif // FCGISERVER_CONSOLELOGCALLBACK_H
