#include "request.h"
#include "router.h"
#include "server.h"
#include <cstdio>

namespace
{

void hello_world(fcgiserver::RequestContext & context)
{
	fcgiserver::Request & request = context.request();
	request.set_content_type("text/html");

	auto stream = request.write_stream();
	stream << "<!DOCTYPE html>\n";
	stream << "<html><body>\n";
	stream << "<h1>Hello world!</h1>\n";

	auto const& params = request.query();
	if (!params.empty())
	{
		stream << "<h2>Query</h2>";
		stream << "<table>\n";
		stream << "<thead><th>Key</th><th>Value</th></thead>\n";
		stream << "<tbody>\n";

		for (auto const& entry : params)
		{
			auto lhs = request.query_decode(entry.first);
			auto rhs = request.query_decode(entry.second);

			stream << "<tr><td>";
			stream << fcgiserver::HTMLContent(lhs.second);
			stream << "</td><td>";
			stream << fcgiserver::HTMLContent(rhs.second);
			stream << "</td></tr>\n";
		}

		stream << "</tbody>\n";
		stream << "</table>\n";
	}

	stream << "<h2>Environment</h2>";
	stream << "<table>\n";
	stream << "<thead><th>Key</th><th>Value</th></thead>\n";
	stream << "<tbody>\n";

	auto const& env_map = request.env_map();
	for (auto & entry : env_map)
	{
		stream << "<tr><td>";
		stream << fcgiserver::HTMLContent(entry.first);
		stream << "</td><td>";
		stream << fcgiserver::HTMLContent(entry.second);
		stream << "</td></tr>\n";
	}

	stream << "</tbody>\n";
	stream << "</table>\n";
	stream << "</body></html>\n";
}

}

int main(int argc, char **argv)
{
	auto router = std::make_shared<fcgiserver::Router>();
	router->add_route(&hello_world, "/", fcgiserver::RequestMethod::GET);

	fcgiserver::Server server;
	server.set_router(router);

	if (!server.initialize("/tmp/fcgiserver.sock"))
		return EXIT_FAILURE;

	if (!server.add_threads(4))
		return EXIT_FAILURE;

	server.wait_for_terminate_signal();
	server.logger().log(fcgiserver::LogLevel::Info, "Initiating shutdown...");

	server.finalize();
	server.logger().log(fcgiserver::LogLevel::Info, "All done!");

	return EXIT_SUCCESS;
}
