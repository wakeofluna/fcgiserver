#include "router.h"
#include "request.h"
#include "test_mock_cgi_data.h"
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace fcgiserver;

namespace
{

using TestPair = std::pair<int,Request::Route>;
using TestPairs = std::vector<TestPair>;

class DummyLogger : public ILogCallback
{
public:
	DummyLogger(std::string & d, std::string & i, std::string & e)
	    : log_debug(d), log_info(i), log_error(e) {}

	void log_message(LogLevel level, std::string_view const& message) override
	{
		switch (level)
		{
			case LogLevel::Debug:
				log_debug.append(message);
				break;
			case LogLevel::Info:
				log_info.append(message);
				break;
			case LogLevel::Error:
				log_error.append(message);
				break;
		}
	}

private:
	std::string & log_debug;
	std::string & log_info;
	std::string & log_error;
};

struct DummyRoutes
{
	Router::Callback operator[] (int index)
	{
		return [this,index](Logger const& logger, Request & request)
		{
			(void)logger;
			calls.push_back({index, request.relative_route()});
		};
	}

	TestPairs calls;

	std::unique_ptr<ILogCallback> logger()
	{
		return std::make_unique<DummyLogger>(log_debug, log_info, log_error);
	}

	std::string log_debug;
	std::string log_info;
	std::string log_error;

	void clear()
	{
		calls.clear();
		log_debug.clear();
		log_info.clear();
		log_error.clear();
	}

};

TestPairs check(int index, std::initializer_list<std::string_view> items)
{
	return TestPairs{
		    TestPair{index, Request::Route(items)}
	};
}

}

static std::ostream& operator<< (std::ostream& stream, TestPair const& pair)
{
	stream << '[' << pair.first << ", \"";
	for (auto const& item : pair.second)
		stream << '/' << item;
	stream << "\"]";
	return stream;
}

#include <catch2/catch_test_macros.hpp>


TEST_CASE("Router-Single router", "[router]")
{
	DummyRoutes dummy_routes;
	Logger logger(dummy_routes.logger());

	Router router;
	router.add_route(dummy_routes[1], "/");
	router.add_route(dummy_routes[2], "/foo");
	router.add_route(dummy_routes[3], "/bar");
	router.add_route(dummy_routes[4], "/down/into/the/deep");
	router.add_route(dummy_routes[5], "/down/we/go");
	router.add_route(dummy_routes[6], "/down/into/lava");

	const char *envp[] = {
	    nullptr,
	    nullptr,
	    nullptr
	};

	MockCgiData cgidata(std::string(), envp);

	SECTION("Root entry")
	{
		{
			envp[0] = nullptr;
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(1, {}));
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(1, {}));
		}
	}

	SECTION("Single depth entries")
	{
		{
			envp[0] = "DOCUMENT_URI=/foo";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(2, {"foo"}));
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/foo/";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(2, {"foo"}));
		}

		dummy_routes.clear();
		{
			envp[0] = "DOCUMENT_URI=/bar";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(3, {"bar"}));
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/evil";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::NotFound );
			REQUIRE( dummy_routes.calls.empty() );
		}

		{
			envp[0] = "DOCUMENT_URI=/down";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::NotFound );
			REQUIRE( dummy_routes.calls.empty() );
		}
	}

	SECTION("Multiple depth entries")
	{
		{
			envp[0] = "DOCUMENT_URI=/foo/nope";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::NotFound );
			REQUIRE( dummy_routes.calls.empty() );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/foo/bar";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::NotFound );
			REQUIRE( dummy_routes.calls.empty() );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/down/into";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::NotFound );
			REQUIRE( dummy_routes.calls.empty() );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/down/into/the";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::NotFound );
			REQUIRE( dummy_routes.calls.empty() );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/down/into/the/deep";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(4, {"down","into","the","deep"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/down/we/go/";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(5, {"down","we","go"}) );
		}
	}

	SECTION("Combinations of methods and overlapping routes")
	{
		router.add_route(dummy_routes[7], "/down/we");
		router.add_route(dummy_routes[8], "/foo", RequestMethod::GET);
		router.add_route(dummy_routes[9], "/foo", RequestMethod::DELETE);
		router.add_route(dummy_routes[10], "/down", RequestMethod::POST);

		{
			envp[0] = "DOCUMENT_URI=/foo";
			envp[1] = "REQUEST_METHOD=GET";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(8, {"foo"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/foo";
			envp[1] = "REQUEST_METHOD=POST";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(2, {"foo"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/foo";
			envp[1] = "REQUEST_METHOD=DELETE";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(9, {"foo"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/down";
			envp[1] = "REQUEST_METHOD=GET";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::InvalidMethod );
			REQUIRE( dummy_routes.calls.empty() );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/down";
			envp[1] = "REQUEST_METHOD=POST";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(10, {"down"}) );
		}
	}

	SECTION("Wildcards")
	{
		// TODO
	}

	SECTION("Catchalls")
	{
		router.add_route(dummy_routes[7], "/api", RequestMethod::CatchAllRecursive);
		router.add_route(dummy_routes[8], "/down/into", RequestMethod::CatchAllRecursive);
		router.add_route(dummy_routes[9], "/down/into/water", RequestMethod::DELETE);

		{
			envp[0] = "DOCUMENT_URI=/down/into/the/deep";
			envp[1] = nullptr;
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(4, {"down","into","the","deep"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/down/into/the/deepest";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(8, {"down","into","the","deepest"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/down/into";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(8, {"down","into"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/down/into/water";
			envp[1] = "REQUEST_METHOD=DELETE";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(9, {"down","into","water"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/down/into/water";
			envp[1] = "REQUEST_METHOD=PUT";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::InvalidMethod );
			REQUIRE( dummy_routes.calls.empty() );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/api/user/WakeOfLuna/posts";
			envp[1] = "REQUEST_METHOD=PUT";
			Request request(cgidata);
			IRouter::RouteResult result = router.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(7, {"api","user","WakeOfLuna","posts"}) );
		}
	}
}

TEST_CASE("Router-Multiple routers", "[router]")
{
	DummyRoutes dummy_routes;
	Logger logger(dummy_routes.logger());

	auto router1 = std::make_shared<Router>();
	router1->add_route(dummy_routes[1], "/");
	router1->add_route(dummy_routes[2], "/foo");
	router1->add_route(dummy_routes[3], "/bar");
	router1->add_route(dummy_routes[4], "/down/into/the/deep");
	router1->add_route(dummy_routes[5], "/down/we/go");
	router1->add_route(dummy_routes[6], "/down/into/lava");

	auto router2 = std::make_shared<Router>();
	router2->add_route(dummy_routes[7], "/");
	router2->add_route(dummy_routes[8], "/we/go");
	router2->add_route(dummy_routes[9], "/into/water");
	router2->add_route(dummy_routes[10], "/monster");

	const char *envp[] = {
	    nullptr,
	    nullptr
	};

	MockCgiData cgidata(std::string(), envp);

	SECTION("Non overlapping sub routers")
	{
		Router root;
		root.add_route(router1, "/api");
		root.add_route(router2, "/work");

		{
			envp[0] = "DOCUMENT_URI=/api";
			Request request(cgidata);
			IRouter::RouteResult result = root.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(1, {}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/api/foo";
			Request request(cgidata);
			IRouter::RouteResult result = root.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(2, {"foo"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/work/";
			Request request(cgidata);
			IRouter::RouteResult result = root.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(7, {}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/bar";
			Request request(cgidata);
			IRouter::RouteResult result = root.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::NotFound );
			REQUIRE( dummy_routes.calls.empty() );
		}
	}

	SECTION("Overlapping sub routers")
	{
		Router root;
		root.add_route(router1, "/api");
		router1->add_route(router2, "/down");
		root.add_route(dummy_routes[11], "/api/down/for/victory");

		{
			envp[0] = "DOCUMENT_URI=/api/down/into/the/deep";
			Request request(cgidata);
			IRouter::RouteResult result = root.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(4, {"down","into","the","deep"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/api/down/monster";
			Request request(cgidata);
			IRouter::RouteResult result = root.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(10, {"monster"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/api/down/for/victory";
			Request request(cgidata);
			IRouter::RouteResult result = root.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(11, {"api","down","for","victory"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/api/down/we/go";
			Request request(cgidata);
			IRouter::RouteResult result = root.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(8, {"we","go"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/api/down/into/water";
			Request request(cgidata);
			IRouter::RouteResult result = root.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(9, {"into","water"}) );
		}

		dummy_routes.clear();

		{
			envp[0] = "DOCUMENT_URI=/api/down/into/lava";
			Request request(cgidata);
			IRouter::RouteResult result = root.handle_request(logger, request);
			REQUIRE( result == IRouter::RouteResult::Handled );
			REQUIRE( dummy_routes.calls == check(6, {"down","into","lava"}) );
		}
	}

	SECTION("Catchall")
	{
		// TODO
	}
}
