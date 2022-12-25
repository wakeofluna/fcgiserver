#include "request.h"
#include "router.h"
#include "server.h"
#include <cstdio>

namespace
{

void hello_world(fcgiserver::Logger const& logger, fcgiserver::Request & request)
{
	request.set_content_type("text/html");

	request.write("<!DOCTYPE html>\n");
	request.write("<html><body>\n");
	request.write("<h1>Hello world!</h1>\n");

	auto const& params = request.query();
	if (!params.empty())
	{
		request.write("<h2>Query</h2>");
		request.write("<table>\n");
		request.write("<thead><th>Key</th><th>Value</th></thead>\n");
		request.write("<tbody>\n");

		for (auto const& entry : params)
		{
			request.write("<tr><td>");

			auto entry_key = request.query_decode(entry.first);
			if (entry_key.first)
				request.write_html(entry_key.second);
			else
				request.write("(invalid sequence)");

			request.write("</td><td>");

			auto entry_value = request.query_decode(entry.second);
			if (entry_value.first)
				request.write_html(entry_value.second);
			else
				request.write("(invalid sequence)");

			request.write("</td></tr>\n");
		}

		request.write("</tbody>\n");
		request.write("</table>\n");
	}

	request.write("<h2>Environment</h2>");
	request.write("<table>\n");
	request.write("<thead><th>Key</th><th>Value</th></thead>\n");
	request.write("<tbody>\n");
	auto const& env_map = request.env_map();
	for (auto & entry : env_map)
	{
		request.write("<tr><td>");
		request.write(entry.first.to_string_view());
		request.write("</td><td>");
		request.write(entry.second);
		request.write("</td></tr>\n");
	}
	request.write("</tbody>\n");
	request.write("</table>\n");

	request.write("</body></html>\n");
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
