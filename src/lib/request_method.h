#ifndef FCGISERVER_REQUEST_METHOD_H
#define FCGISERVER_REQUEST_METHOD_H

#include <cstdint>

namespace fcgiserver
{

enum class RequestMethod : std::uint8_t
{
	UNKNOWN,
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	CONNECT,
	OPTIONS,
	TRACE,
	PATCH,
	OTHER,
};

} // namespace fcgiserver

#endif // FCGISERVER_REQUEST_METHOD_H
