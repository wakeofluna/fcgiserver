#include "server.h"
#include "request.h"
#include "request_context.h"
#include "request_context_private.h"
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
#include <shared_mutex>


using namespace fcgiserver;


namespace
{

thread_local volatile bool shutdown_triggered = false;
thread_local volatile bool tick_triggered = false;

// Signal sent to gracefully terminate a thread
void sigusr1(int signum, siginfo_t *info, void *ucontext)
{
	shutdown_triggered = true;
}

// Signal sent to threads to tick their context
void sigusr2(int signum, siginfo_t *info, void *ucontext)
{
	tick_triggered = true;
}

// Tiny default router
class EmptyRouter : public fcgiserver::IRouter
{
public:
	EmptyRouter() {}
	~EmptyRouter() {}

	RouteResult handle_request(fcgiserver::RequestContext & context) override
	{
		fcgiserver::Request & request = context.request();
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


class fcgiserver::ServerPrivate
{
public:
	ServerPrivate()
	    : socket_fd(0)
	    , last_thread_id(0)
	{}

	std::mutex threads_lock;
	std::shared_mutex context_lock;
	std::string socket_path;
	int socket_fd;
	Logger logger;
	std::shared_ptr<IRouter> router;
	std::list<std::thread> threads;
	size_t last_thread_id;
	std::shared_ptr<UserContext> global_context;
	std::function<UserContext*(std::shared_ptr<UserContext> const&)> create_thread_context;
	std::chrono::seconds thread_context_tick_interval;
};


Server::Server()
    : m_private(new ServerPrivate)
{
	m_private->logger.set_log_callback(std::make_unique<ConsoleLogCallback>());
	m_private->thread_context_tick_interval = std::chrono::seconds(60);
}

Server::~Server()
{
	finalize();
	delete m_private;
}

Logger & Server::logger()
{
	return m_private->logger;
}

void Server::set_router(std::shared_ptr<IRouter> router)
{
	if (!router)
	{
		m_private->logger.log(LogLevel::Error, "Attempted to register an invalid router");
		return;
	}

	std::lock_guard<std::shared_mutex> guard(m_private->context_lock);
	m_private->router = router;
}

void Server::set_global_context(std::shared_ptr<UserContext> const& new_context)
{
	std::lock_guard<std::shared_mutex> guard(m_private->context_lock);
	m_private->global_context = new_context;
}

void Server::set_thread_context(std::function<UserContext*(std::shared_ptr<UserContext> const&)> && create_context_function)
{
	std::lock_guard<std::shared_mutex> guard(m_private->context_lock);
	m_private->create_thread_context = std::move(create_context_function);
}

void Server::set_thread_context_tick_interval(std::chrono::seconds duration)
{
	m_private->thread_context_tick_interval = duration;
}

bool Server::initialize(std::string socket_path)
{
	int sockfd;

	int result = FCGX_Init();
	if (result != 0)
	{
		m_private->logger.error() << "Error " << result << " on FCGX_Init";
		return false;
	}

	if (FCGX_IsCGI())
	{
		// Running standalone, need to create our own socket
		if (socket_path.empty())
		{
			m_private->logger.log(LogLevel::Error, "Running as CGI but no socket_path given!");
			return false;
		}

		umask(0);

		sockfd = FCGX_OpenSocket(socket_path.c_str(), 10);
		if (sockfd == -1)
		{
			m_private->logger.error() << "Create socket error: " << strerror(errno);
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
	{
		std::lock_guard<std::shared_mutex> guard(m_private->context_lock);
		if (!m_private->router)
		{
			m_private->logger.log(LogLevel::Error, "No router provided, adding an empty route");
			m_private->router.reset(new EmptyRouter());
		}
	}

	m_private->logger.info() << "Starting " << count << " threads";

	std::lock_guard<std::mutex> guard(m_private->threads_lock);

	// Add ticker thread
	if (m_private->threads.empty())
		m_private->threads.emplace_back(&Server::run_thread_tick_function, this);

	for (size_t i = 0; i < count; ++i)
		m_private->threads.emplace_back(&Server::run_thread_function, this, ++m_private->last_thread_id);

	return true;
}

void Server::kill_threads()
{
	std::lock_guard<std::mutex> guard(m_private->threads_lock);

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

void Server::install_thread_signal_handlers()
{
	sigset_t sigset;
	sigfillset(&sigset);
	sigdelset(&sigset, SIGUSR1);
	sigdelset(&sigset, SIGUSR2);
	pthread_sigmask(SIG_SETMASK, &sigset, nullptr);

	struct sigaction sa = {};
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = &sigusr1;
	sigaction(SIGUSR1, &sa, nullptr);
	sa.sa_sigaction = &sigusr2;
	sigaction(SIGUSR2, &sa, nullptr);
}

void Server::run_thread_function(Server * server, size_t id)
{
	install_thread_signal_handlers();
	server->thread_function(id);
}

void Server::thread_function(size_t id)
{
	int result;
	FCGX_Request fcgx_request;

	result = FCGX_InitRequest(&fcgx_request, m_private->socket_fd, FCGI_FAIL_ACCEPT_ON_INTR);
	if (result != 0)
	{
		m_private->logger.error() << "Error " << result << " on FCGX_InitRequest: " << strerror(-result);
		return;
	}

	fcgiserver::RequestContext context;
	{
		std::lock_guard<std::shared_mutex> guard(m_private->context_lock);
		context.m_private->thread_id = id;
		context.m_private->server = this;
		if (m_private->create_thread_context)
			context.m_private->thread_context.reset(
			                m_private->create_thread_context(m_private->global_context)
			            );
	}

	m_private->logger.debug() << "Thread #" << id << " started";

	while (!shutdown_triggered)
	{
		// Time to do some maintenance?
		if (tick_triggered)
		{
			tick_triggered = false;
			if (context.m_private->thread_context)
				context.m_private->thread_context->tick();
		}

		result = FCGX_Accept_r(&fcgx_request);
		if (result != 0)
		{
			if (result == -9999)
			{
				// Regular exit
				shutdown_triggered = true;
				break;
			}
			else if (result != -EINTR)
			{
				m_private->logger.error() << "Error " << result << " on FCGX_Accept_r: " << strerror(-result);
				break;
			}
			else
			{
				continue;
			}
		}

		std::shared_ptr<fcgiserver::IRouter> router;
		{
			std::shared_lock<std::shared_mutex> guard(m_private->context_lock);
			context.m_private->global_context = m_private->global_context;
			router = m_private->router;
		}

		fcgiserver::FastCgiData fcgi_data(fcgx_request);
		fcgiserver::Request request(fcgi_data, m_private->logger);
		context.m_private->request = &request;

		IRouter::RouteResult route_result = IRouter::RouteResult::InternalError;
		try
		{
			route_result = router->handle_request(context);
		}
		catch (std::exception & exc)
		{
			m_private->logger.error() << "Uncaught exception in thread " << id << ": " << exc.what() << " - "<< request.request_method_string() << ' ' << request.document_uri();;
		}
		catch (...)
		{
			m_private->logger.error() << "Uncaught unknown exception in thread " << id << " - "<< request.request_method_string() << ' ' << request.document_uri();;;
		}

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
				case IRouter::RouteResult::InternalError:
					request.set_http_status(500);
					break;
			}
		}

		// Make sure the headers are sent
		if (!request.headers_sent())
		{
			// TODO Need some form of proper default pages?
			if (request.content_type().empty())
			{
				request.set_content_type("text/plain");
				request.write_stream() << request.http_status();
			}
			else
			{
				request.send_headers();
			}
		}

		// Log the request/result
		if (auto * cb = m_private->logger.log_callback(); cb)
			cb->log_request(request);

		// Process context updates
		if (context.m_private->replaced_global_context)
		{
			context.m_private->replaced_global_context = false;
			std::lock_guard<std::shared_mutex> guard(m_private->context_lock);
			m_private->global_context = context.m_private->global_context;
		}
	}

	m_private->logger.debug() << "Thread #" << id << " finished";
}

void Server::run_thread_tick_function(Server * server)
{
	install_thread_signal_handlers();
	server->thread_tick_function();
}

void Server::thread_tick_function()
{
	using clock = std::chrono::steady_clock;
	size_t trigger_index = size_t(-1);
	size_t num_threads;

	m_private->logger.debug() << "Maintenance thread started";

	std::unique_lock<std::mutex> guard(m_private->threads_lock);
	num_threads = m_private->threads.size();
	guard.unlock();

	while (!shutdown_triggered)
	{
		// Have to manually sleep using nanosleep instead of sleep_until since
		// glibc loops while EINTR which is not what we want here
		auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(m_private->thread_context_tick_interval) / (num_threads > 1 ? num_threads-1 : 1);
		auto seconds = std::chrono::floor<std::chrono::seconds>(remaining);
		auto nanoseconds = std::chrono::floor<std::chrono::nanoseconds>(remaining - seconds);
		struct timespec ts = {
			static_cast<std::time_t>(seconds.count()),
			static_cast<long>(nanoseconds.count()),
		};

		auto endpoint = clock::now() + remaining;
		while (!shutdown_triggered && clock::now() < endpoint)
			::nanosleep(&ts, &ts);

		// Using try_lock to avoid deadlocks during shutdown
		if (!shutdown_triggered && guard.try_lock())
		{
			// Hard-assuming that we are in the list and on index 0 so we skip it
			// Could in theory tick the global context but that is never threadsafe so we don't do it
			num_threads = m_private->threads.size();
			trigger_index = (trigger_index + 1) % num_threads;
			if (trigger_index == 0)
				trigger_index = 1;

			auto iter = std::next(m_private->threads.begin(), trigger_index);
			if (iter != m_private->threads.end())
				pthread_kill(iter->native_handle(), SIGUSR2);

			guard.unlock();
		}
	}

	m_private->logger.debug() << "Maintenance thread finished";
}

