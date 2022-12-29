#ifndef FCGISERVER_REQUEST_CONTEXT_PRIVATE_H
#define FCGISERVER_REQUEST_CONTEXT_PRIVATE_H

#include <memory>
#include "fcgiserver_defs.h"

namespace fcgiserver
{

class Logger;
class Request;
class Server;
class UserContext;

class DLL_PRIVATE RequestContextPrivate
{
public:
	RequestContextPrivate()
	    : thread_id(0)
	    , server(nullptr)
	    , request(nullptr)
	    , replaced_global_context(false)
	{}

	size_t thread_id;
	Server const* server;
	Request * request;
	std::shared_ptr<UserContext> global_context;
	std::unique_ptr<UserContext> thread_context;
	bool replaced_global_context;
};

}

#endif // FCGISERVER_REQUEST_CONTEXT_PRIVATE_H
