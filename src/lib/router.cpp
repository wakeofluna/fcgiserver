#include "router.h"

using namespace fcgiserver;

Router::Router()
{

}

Router::~Router()
{

}

void fcgiserver::Router::handle_request(const LogCallback & logger, Request & request)
{
}

bool Router::add_route(const std::string_view & route, std::shared_ptr<IRouter> router)
{
	return false;
}

bool Router::add_route(const std::string_view & route, Callback && callback)
{
	return false;
}

bool Router::add_route(RequestMethod method, const std::string_view & route, Callback && callback)
{
	return false;
}

bool Router::remove_route(const std::string_view & route)
{
	return false;
}
