fcgiserver
==========
A convenience C++ library that wraps around the existing fastcgi library,
providing a bunch of easy-to-use classes to quickly set up and run your own
webservice written fully in C++.

The point of this library is to enable developers to create blazingly fast
native webservices without having to rely on interpreters such as python,
ruby or php, or being stuck with ASP or Java servers/intermediaries.

Although you can do anything you want and render html pages fully in code,
it is probably more useful to link with a json library and create REST-like
interfaces.

Created by Esther Dalhuisen (wakeofluna@astralkey.nl).

License: [GPL-2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html)

Original source: https://github.com/wakeofluna/fcgiserver

Functions
=========
The core functionality of this library consists of just 2 classes:

1. Server : primary class that interacts with the fastcgi library, accepts
   and dispatches requests, and makes sure everything is handled until
   completion.

2. Request : per-request class that provides access to all the request-specific
   data such as the request URI, query parameters, request method, and takes
   care of reading from the FCGI Request and writing to the FCGI Response.

Convenience functions
---------------------
Apart from the core functionality, the library provides a set of convenience
classes and functions to make your life easier and help you develop faster
and cleaner webservices.

Some of the more notable convenience functions are:

1. Router : helps you route URI's and RequestMethods to functions using
   callback registration as is common in other frameworks.

2. LogCallback : a hook into a logger that is passed to every request.

Future plans
------------
Although the library is already usable, future plans for the library probably
involve more convenience functions to deal with POST data and json streaming.

Usage
=====
Below is a fully working example that will run 4 service threads and listens
to the `GET /` request. It consists of the following parts:

* Create a router and add the desired route(s)
* Instantiate the server and supply the router
* Let the server initialize fastcgi and setup the IPC socket
* Start 4 service threads (these will start immediately)

Then for convenience, the server provides a helper function to wait for a
terminate signal, which is either SIGINT, SIGTERM or SIGHUP. This allows the
server to play nice when run from both the console (for testing) or under an
init-process such as systemd.

* Finalizing will stop accepting new connections and stop all service threads

```C++
#include <fcgiserver/request.h>
#include <fcgiserver/router.h>
#include <fcgiserver/server.h>
#include <cstdio>

namespace
{

void hello_world(fcgiserver::LogCallback const& logger, fcgiserver::Request & request)
{
    request.set_content_type("text/html");
    request.write("<!DOCTYPE html>\n");
    request.write("<html><body>\n");
    request.write("<h1>Hello world!</h1>\n");
    request.write("</body></html>\n");
}

}

int main(int argc, char **argv)
{
    auto router = std::make_shared<fcgiserver::Router>();
    router->add_route(&hello_world, "/", fcgiserver::RequestMethod::GET);

    fcgiserver::Server server;
    server.set_router(router);

    if (!server.initialize("/run/fcgiserver.sock"))
        return EXIT_FAILURE;

    if (!server.add_threads(4))
        return EXIT_FAILURE;

    server.wait_for_terminate_signal();
    server.log(fcgiserver::LogLevel::Info, "Initiating shutdown...\n");

    server.finalize();
    server.log(fcgiserver::LogLevel::Info, "All done!\n");

    return EXIT_SUCCESS;
}
```

CMake
-----
The above example can be compiled using a simple CMake setup such as:
```
cmake_minimum_required(VERSION 3.21)
project(fcgiserver_demo CXX)
find_package(fcgiserver REQUIRED)
add_executable(fcgiserver_demo main.cpp)
target_link_libraries(fcgiserver_demo PRIVATE fcgiserver)
```
Setup
-----
The easiest way to setup a compiled server is to hook it into an existing
NGINX server.

First you create an upstream definition such as:
```
http {
    ...
    upstream fcgiserver {
        server unix:/run/fcgiserver.sock fail_timeout=0;
    }
    ...
}
```

Followed by a server definition that references this upstream such as:
```
http {
    ...
    server {
        listen 443;
        server_name fcgiserver.lan;

        location / {
            fastcgi_pass fcgiserver;
            include fastcgi_params;
        }
    }
    ...
}
```

The `fastcgi_params` file here is a file that is supplied by nginx.
You should not modify the existing entries in there, but you are free
to add new entries that you can then easily access inside your fcgiserver.
