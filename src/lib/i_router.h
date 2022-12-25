#ifndef FCGISERVER_I_ROUTER_H
#define FCGISERVER_I_ROUTER_H

#include "fcgiserver_defs.h"
#include "logger.h"

namespace fcgiserver
{

class Request;

class DLL_PUBLIC IRouter
{
public:
	enum class RouteResult
	{
		Handled,
		NotFound,
		InvalidMethod,
	};

public:
	virtual ~IRouter() = default;
	virtual RouteResult handle_request(fcgiserver::Logger const& logger, fcgiserver::Request & request) = 0;
};

} // namespace fcgiserver

#endif // FCGISERVER_I_ROUTER_H
