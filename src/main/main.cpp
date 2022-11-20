#include "request.h"
#include "server.h"
#include <cstdio>
#include <fcgiapp.h>

using namespace std::literals::string_view_literals;

namespace
{

void hello_world(fcgiserver::LogCallback const& logger, fcgiserver::Request & request)
{
	std::string_view uri = request.document_uri();

	if (uri != "/"sv)
	{
		request.set_http_status(404);
		return;
	}

	request.write("<html><body>\n");
	request.write("<h1>Hello world!</h1>\n");

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
		request.write("</td></tr>");
	}
	request.write("</tbody>\n");
	request.write("</table>\n");

	request.write("</body></html>\n");
}

}

int main(int argc, char **argv)
{
	fcgiserver::Server server;

	server.set_callback(hello_world);

	if (!server.initialize("/tmp/fcgiserver.sock"))
		return EXIT_FAILURE;

	if (!server.add_threads(4))
		return EXIT_FAILURE;

	server.wait_for_terminate_signal();
	server.log(fcgiserver::LogLevel::Info, "Initiating shutdown...\n");

	server.finalize();
	server.log(fcgiserver::LogLevel::Info, "All done!\n");

	return EXIT_SUCCESS;
}
