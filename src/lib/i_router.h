#ifndef FCGISERVER_I_ROUTER_H
#define FCGISERVER_I_ROUTER_H

#include "fcgiserver_defs.h"

namespace fcgiserver
{

class RequestContext;

class DLL_PUBLIC IRouter
{
public:
	enum class RouteResult
	{
		Handled,
		NotFound,
		InvalidMethod,
		InternalError,
	};

public:
	virtual ~IRouter() = default;
	virtual RouteResult handle_request(fcgiserver::RequestContext & context) = 0;
};

} // namespace fcgiserver

#endif // FCGISERVER_I_ROUTER_H
