#ifndef FCGISERVER_ROUTER_H
#define FCGISERVER_ROUTER_H

#include "fcgiserver_defs.h"
#include "i_router.h"
#include "request_method.h"
#include <functional>
#include <memory>
#include <string_view>

namespace fcgiserver
{

class RouterPrivate;
class DLL_PUBLIC Router : public IRouter
{
public:
	using Callback = std::function<void(LogCallback const&, Request&)>;

public:
	Router();
	~Router();

	IRouter::RouteResult handle_request(LogCallback const& logger, Request & request) override;

	void add_route(std::shared_ptr<IRouter> router, std::string_view const& route);
	void add_route(Callback && callback, std::string_view const& route, RequestMethod method = RequestMethod::CatchAllHere);
	bool remove_route(std::string_view const& route);

protected:
	RouterPrivate * m_private;
};

}

#endif // FCGISERVER_ROUTER_H
