#include "server.h"
#include "request.h"
#include "fast_cgi_data.h"
#include "i_router.h"
#include "console_log_callback.h"
#include "logger.h"

#include <cstdarg>
#include <cerrno>
#include <cstring>
#include <fcgiapp.h>
#include <list>
#include <thread>
#include <signal.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <mutex>


namespace
{

// Empty function to prevent default action TERM on signal
void sigusr1(int signum, siginfo_t *info, void *ucontext)
{
}

// Tiny default router
class EmptyRouter : public fcgiserver::IRouter
{
public:
	EmptyRouter() {}
	~EmptyRouter() {}

	RouteResult handle_request(fcgiserver::Logger const& logger, fcgiserver::Request & request) override
	{
		std::string_view uri = request.document_uri();
		if (uri != "/"sv)
			return RouteResult::NotFound;

		request.set_content_type("text/html");
		request.write("<!DOCTYPE html>\n");
		request.write("<html lang=\"en\">\n");
		request.write("<head><meta charset=\"utf-8\" /><title>FCGIserver online</title></head>\n");
		request.write("<body>\n");
		request.write("<h1>Hello world</h1>\n");
		request.write("Congratulations! Your server is working. Now it is time to register a router and add some content!\n");
		request.write("</body>\n</html>\n");
		return RouteResult::Handled;
	}
};

}


namespace fcgiserver
{

class ServerPrivate
{
public:
	ServerPrivate() : socket_fd(0) {}

	inline void log(LogLevel lvl, const char * msg) const
	{
		logger.log(lvl, msg);
	}

	void logf(LogLevel lvl, const char * fmt, ...) const
	{
		std::va_list vl;
		va_start(vl, fmt);
		logger.stream(lvl).vprintf(fmt, vl);
		va_end(vl);
	}

	std::string socket_path;
	int socket_fd;
	Logger logger;
	std::shared_ptr<IRouter> router;
	std::list<std::thread> threads;
};



Server::Server()
    : m_private(new ServerPrivate)
{
	m_private->logger.set_log_callback(std::make_unique<ConsoleLogCallback>());
}

Server::~Server()
{
	finalize();
	delete m_private;
}

void Server::set_router(std::shared_ptr<IRouter> router)
{
	if (!router)
	{
		m_private->log(LogLevel::Error, "Attempted to register an invalid router");
		return;
	}
	m_private->router = router;
}

Logger & Server::logger()
{
	return m_private->logger;
}

bool Server::initialize(std::string socket_path)
{
	int sockfd;

	int result = FCGX_Init();
	if (result != 0)
	{
		m_private->logf(LogLevel::Error, "Error %d on FCGX_Init!", result);
		return false;
	}

	if (FCGX_IsCGI())
	{
		// Running standalone, need to create our own socket
		if (socket_path.empty())
		{
			m_private->log(LogLevel::Error, "Running as CGI but no socket_path given!");
			return false;
		}

		umask(0);

		sockfd = FCGX_OpenSocket(socket_path.c_str(), 10);
		if (sockfd == -1)
		{
			m_private->logf(LogLevel::Error, "Create socket error : %s", strerror(errno));
			return false;
		}

		m_private->socket_path = std::move(socket_path);
	}
	else
	{
		// Running in webserver context, accept it as reality
		sockfd = 0;
	}

	m_private->socket_fd = sockfd;
	return true;
}

bool Server::add_threads(size_t count)
{
	if (!m_private->router)
	{
		m_private->log(LogLevel::Error, "No router provided, adding an empty route");
		m_private->router.reset(new EmptyRouter());
	}

	m_private->logf(LogLevel::Info, "Starting %u threads", count);

	for (unsigned int i = 0; i < count; ++i)
		m_private->threads.emplace_back(&Server::run_thread_function, this);

	return true;
}

void Server::kill_threads()
{
	std::for_each(m_private->threads.begin(), m_private->threads.end(), [] (std::thread & th) {
		if (th.joinable())
			pthread_kill(th.native_handle(), SIGUSR1);
	});

	std::for_each(m_private->threads.begin(), m_private->threads.end(), [] (std::thread & th) {
		if (th.joinable())
			th.join();
	});

	m_private->threads.clear();
}

void Server::finalize()
{
	FCGX_ShutdownPending();

	kill_threads();

	if (m_private->socket_fd != 0)
	{
		::close(m_private->socket_fd);
		::unlink(m_private->socket_path.c_str());
		m_private->socket_fd = 0;
		m_private->socket_path.clear();
	}
}

int Server::wait_for_terminate_signal() const
{
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGTERM);
	sigaddset(&sigset, SIGHUP);
	pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

	int signum = 0;
	sigwait(&sigset, &signum);
	pthread_sigmask(SIG_UNBLOCK, &sigset, nullptr);

	return signum;
}

void Server::run_thread_function(Server * server)
{
	server->thread_function();
}

void Server::thread_function()
{
	{
		sigset_t sigset;
		sigfillset(&sigset);
		sigdelset(&sigset, SIGUSR1);
		pthread_sigmask(SIG_SETMASK, &sigset, nullptr);

		struct sigaction sa = {};
		sa.sa_flags = SA_SIGINFO;
		sa.sa_sigaction = &sigusr1;
		sigemptyset(&sa.sa_mask);
		sigaction(SIGUSR1, &sa, nullptr);
	}

	int result;
	FCGX_Request fcgx_request;

	result = FCGX_InitRequest(&fcgx_request, m_private->socket_fd, 0);
	if (result != 0)
	{
		m_private->logf(LogLevel::Error, "Error %d on FCGX_InitRequest! (%s)", result, strerror(-result));
		return;
	}

	m_private->log(LogLevel::Info, "Thread started");

	while (true)
	{
		result = FCGX_Accept_r(&fcgx_request);
		if (result != 0)
		{
			if (result != -EINTR)
				m_private->logf(LogLevel::Error, "Error %d on FCGX_Accept_r! (%s)", result, strerror(-result));
			break;
		}

		fcgiserver::FastCgiData fcgi_data(fcgx_request);
		fcgiserver::Request request(fcgi_data);

		IRouter::RouteResult route_result = m_private->router->handle_request(m_private->logger, request);
		if (request.http_status().empty())
		{
			switch (route_result)
			{
				case IRouter::RouteResult::Handled:
					request.set_http_status(200);
					break;
				case IRouter::RouteResult::NotFound:
					request.set_http_status(404);
					break;
				case IRouter::RouteResult::InvalidMethod:
					request.set_http_status(405);
					break;
			}
		}

		// Make sure the headers are sent
		request.send_headers();

		// Log the request/result
		if (auto * cb = m_private->logger.log_callback(); cb)
			cb->log_request(request);
	}

	m_private->log(LogLevel::Info, "Thread finished");
}


}
