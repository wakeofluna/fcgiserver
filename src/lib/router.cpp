#include "router.h"
#include "request.h"
#include "request_method.h"
#include "symbol.h"
#include "symbols.h"
#include <cassert>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <vector>

using namespace fcgiserver;
using namespace std::literals::string_view_literals;


namespace
{

struct SubRoute
{
	SubRoute() = default;
	~SubRoute() = default;

	SubRoute * get_or_create_subroute(Symbol symbol)
	{
		auto & ptr = routes[symbol];
		if (!ptr)
			ptr.reset(new SubRoute);
		return ptr.get();
	}

	SubRoute * maybe_get_subroute(Symbol symbol) const
	{
		auto const& iter = routes.find(symbol);
		return (iter == routes.cend())? nullptr : iter->second.get();
	}

	SubRoute * maybe_get_subroute(std::string_view const& component) const
	{
		return maybe_get_subroute(Symbol::maybe(component));
	}

	bool has_catch_all_recursive() const
	{
		return endpoints.find(RequestMethod::CatchAllRecursive) != endpoints.cend();
	}

	std::unordered_map<Symbol,std::unique_ptr<SubRoute>> routes;
	std::shared_ptr<IRouter> router;
	std::map<RequestMethod,Router::Callback> endpoints;
};

std::string_view find_route_start(std::string_view const& route)
{
	size_t start = route.find_first_not_of('/');
	return (start == std::string_view::npos) ? std::string_view() : route.substr(start);
}

std::string_view split_first_component(std::string_view & route)
{
	assert(!route.empty());
	assert(route[0] != '/');

	size_t end = route.find_first_of('/');
	size_t next = route.find_first_not_of('/', end);

	std::string_view answer = route.substr(0, end);
	route = (next == std::string_view::npos) ? std::string_view() : route.substr(next);
	return answer;
}

}


class fcgiserver::RouterPrivate
{
public:
	RouterPrivate() {}
	~RouterPrivate() = default;

	SubRoute route;
	std::shared_mutex route_mutex;
};


Router::Router()
    : m_private(new RouterPrivate)
{
}

Router::~Router()
{
	delete m_private;
}

IRouter::RouteResult Router::handle_request(Logger const& logger, Request & request)
{
	std::shared_lock<std::shared_mutex> lock(m_private->route_mutex);

	RouteResult route_result = RouteResult::NotFound;

	SubRoute * subroute = &m_private->route;
	SubRoute * last_with_catch_recursive = subroute->has_catch_all_recursive() ? subroute : nullptr;

	Request::Route const& route = request.relative_route();
	for (auto iter = route.cbegin(), iter_end = route.cend(); ; ++iter)
	{
		if (subroute->router)
		{
			Request::Route relative_route(iter, iter_end);
			request.swap_relative_route(relative_route);

			auto new_result = subroute->router->handle_request(logger, request);
			if (new_result != RouteResult::NotFound)
				route_result = new_result;
			if (route_result == RouteResult::Handled)
				return route_result;

			request.swap_relative_route(relative_route);
		}

		if (iter == iter_end)
			break;

		SubRoute * next = subroute->maybe_get_subroute(*iter);
		if (!next)
			next = subroute->maybe_get_subroute(symbols::wildcard);

		subroute = next;
		if (!subroute)
			break;

		if (subroute->has_catch_all_recursive())
			last_with_catch_recursive = subroute;
	}

	if (subroute)
	{
		if (subroute->endpoints.empty())
		{
			route_result = RouteResult::NotFound;
		}
		else
		{
			RequestMethod method = request.request_method();

			auto iter = subroute->endpoints.find(method);
			if (iter == subroute->endpoints.cend())
				iter = subroute->endpoints.find(RequestMethod::CatchAllHere);
			if (iter == subroute->endpoints.cend())
				iter = subroute->endpoints.find(RequestMethod::CatchAllRecursive);

			if (iter != subroute->endpoints.cend())
			{
				iter->second(logger, request);
				return RouteResult::Handled;
			}

			route_result = RouteResult::InvalidMethod;
		}
	}

	if (route_result == RouteResult::NotFound && last_with_catch_recursive)
	{
		auto iter = last_with_catch_recursive->endpoints.find(RequestMethod::CatchAllRecursive);
		if (iter != last_with_catch_recursive->endpoints.cend())
		{
			iter->second(logger, request);
			return RouteResult::Handled;
		}
	}

	return route_result;
}

void Router::add_route(std::shared_ptr<IRouter> router, std::string_view const& route)
{
	std::lock_guard<std::shared_mutex> guard(m_private->route_mutex);

	std::string_view remaining = find_route_start(route);
	SubRoute * subroute = &m_private->route;
	while (!remaining.empty())
	{
		std::string_view component = split_first_component(remaining);
		subroute = subroute->get_or_create_subroute(component);
	}

	subroute->router = router;
}

void Router::add_route(Callback && callback, std::string_view const& route, RequestMethod method)
{
	std::lock_guard<std::shared_mutex> guard(m_private->route_mutex);

	std::string_view remaining = find_route_start(route);
	SubRoute * subroute = &m_private->route;
	while (!remaining.empty())
	{
		std::string_view component = split_first_component(remaining);
		subroute = subroute->get_or_create_subroute(component);
	}

	subroute->endpoints[method] = std::move(callback);
}

bool Router::remove_route(std::string_view const& route)
{
	std::lock_guard<std::shared_mutex> guard(m_private->route_mutex);

	std::string_view remaining = find_route_start(route);
	SubRoute * subroute = &m_private->route;
	while (!remaining.empty())
	{
		std::string_view component = split_first_component(remaining);
		subroute = subroute->get_or_create_subroute(component);
	}

	bool removed = false;
	if (subroute->router)
	{
		removed = true;
		subroute->router.reset();
	}
	if (!subroute->endpoints.empty())
	{
		removed = true;
		subroute->endpoints.clear();
	}
	return removed;
}
