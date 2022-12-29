#include "request_context.h"
#include "request_context_private.h"
#include "request.h"
#include <cassert>


using namespace fcgiserver;


RequestContext::RequestContext()
    : m_private(new RequestContextPrivate)
{
}

RequestContext::RequestContext(Request & req)
    : m_private(new RequestContextPrivate)
{
	m_private->request = &req;
}

RequestContext::~RequestContext()
{
	delete m_private;
}

Server const* RequestContext::server() const
{
	return m_private->server;
}

Logger const& RequestContext::logger() const
{
	assert(m_private->request);
	return m_private->request->logger();
}

Request & RequestContext::request() const
{
	assert(m_private->request);
	return *m_private->request;
}

std::shared_ptr<UserContext> RequestContext::global_context() const
{
	return m_private->global_context;
}

UserContext * RequestContext::thread_context() const
{
	return m_private->thread_context.get();
}

void RequestContext::replace_global_context(std::shared_ptr<UserContext> const& new_context)
{
	m_private->global_context = new_context;
	m_private->replaced_global_context = true;
}

void RequestContext::replace_thread_context(std::unique_ptr<UserContext> && new_context)
{
	m_private->thread_context = std::move(new_context);
}
