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

class DLL_PUBLIC Router : public IRouter
{
public:
	using Callback = std::function<void(LogCallback const&, Request&)>;

public:
	Router();
	~Router();

	void handle_request(LogCallback const& logger, Request & request) override;

	bool add_route(std::string_view const& route, std::shared_ptr<IRouter> router);
	bool add_route(std::string_view const& route, Callback && callback);
	bool add_route(RequestMethod method, std::string_view const& route, Callback && callback);

	bool remove_route(std::string_view const& route);
};

}

#endif // FCGISERVER_ROUTER_H
