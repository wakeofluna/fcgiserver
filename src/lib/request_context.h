#ifndef FCGISERVER_REQUEST_CONTEXT_H
#define FCGISERVER_REQUEST_CONTEXT_H

#include "fcgiserver_defs.h"
#include "user_context.h"
#include <memory>

namespace fcgiserver
{

class Logger;
class Request;
class Server;
class RequestContextPrivate;

class DLL_PUBLIC RequestContext
{
private:
	friend class Server;
	RequestContext();

public:
	explicit RequestContext(Request & req);
	~RequestContext();

	Server const* server() const;
	Logger const& logger() const;
	Request & request() const;
	std::shared_ptr<UserContext> global_context() const;
	UserContext * thread_context() const;

	void replace_global_context(std::shared_ptr<UserContext> const& new_context);
	void replace_thread_context(std::unique_ptr<UserContext> && new_context);

private:
	RequestContextPrivate * m_private;
};

}

#endif // FCGISERVER_REQUEST_CONTEXT_H
