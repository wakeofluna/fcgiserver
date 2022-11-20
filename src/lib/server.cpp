#include "server.h"
#include "request.h"
#include "fast_cgi_data.h"

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

// Default streams logger function
void default_logger(fcgiserver::LogLevel lvl, const char *msg)
{
	static std::mutex mutex;

	std::lock_guard<std::mutex> guard(mutex);

	switch (lvl)
	{
		case fcgiserver::LogLevel::Debug:
			std::fputs("[DEBUG] ", stdout);
			std::fputs(msg, stdout);
			std::fflush(stdout);
			break;
		case fcgiserver::LogLevel::Info:
			std::fputs("[INFO] ", stdout);
			std::fputs(msg, stdout);
			std::fflush(stdout);
			break;
		case fcgiserver::LogLevel::Error:
			std::fputs("[ERROR] ", stderr);
			std::fputs(msg, stderr);
			std::fflush(stderr);
			break;
	}
}

}


namespace fcgiserver
{

class ServerPrivate
{
public:
	ServerPrivate() : socket_fd(0) {}

	void log(LogLevel lvl, const char * msg) const
	{
		if (log_callback)
			log_callback(lvl, msg);
	}

	void logf(LogLevel lvl, const char * fmt, ...) const
	{
		if (log_callback)
		{
			char buffer[512];

			std::va_list vl;
			va_start(vl, fmt);
			std::vsnprintf(buffer, sizeof(buffer), fmt, vl);
			va_end(vl);

			log_callback(lvl, buffer);
		}
	}

	std::string socket_path;
	int socket_fd;
	LogCallback log_callback;
	Server::Callback callback;
	std::list<std::thread> threads;
};



Server::Server()
    : m_private(new ServerPrivate)
{
	m_private->log_callback = &default_logger;
}

Server::~Server()
{
	finalize();
	delete m_private;
}

void Server::set_callback(Callback && callback)
{
	m_private->callback = std::move(callback);
}

void Server::set_log_callback(LogCallback && callback)
{
	m_private->log_callback = std::move(callback);
}

void Server::log(LogLevel lvl, const char * msg) const
{
	m_private->log(lvl, msg);
}

bool Server::initialize(std::string socket_path)
{
	int sockfd;

	int result = FCGX_Init();
	if (result != 0)
	{
		m_private->logf(LogLevel::Error, "Error %d on FCGX_Init!\n", result);
		return false;
	}

	if (FCGX_IsCGI())
	{
		// Running standalone, need to create our own socket
		if (socket_path.empty())
		{
			m_private->log(LogLevel::Error, "Running as CGI but no socket_path given!\n");
			return false;
		}

		umask(0);

		sockfd = FCGX_OpenSocket(socket_path.c_str(), 10);
		if (sockfd == -1)
		{
			m_private->logf(LogLevel::Error, "Create socket error : %s\n", strerror(errno));
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
	if (!m_private->callback)
	{
		m_private->log(LogLevel::Error, "Cannot add threads, no callback function registered\n");
		return false;
	}

	m_private->logf(LogLevel::Info, "Starting %u threads\n", count);

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
		m_private->logf(LogLevel::Error, "Error %d on FCGX_InitRequest! (%s)\n", result, strerror(-result));
		return;
	}

	m_private->log(LogLevel::Info, "Thread started\n");

	while (true)
	{
		result = FCGX_Accept_r(&fcgx_request);
		if (result != 0)
		{
			if (result != -EINTR)
				m_private->logf(LogLevel::Error, "Error %d on FCGX_Accept_r! (%s)\n", result, strerror(-result));
			break;
		}

		fcgiserver::FastCgiData fcgi_data(fcgx_request);
		fcgiserver::Request request(fcgi_data);

		m_private->callback(m_private->log_callback, request);

		// Make sure the headers are sent
		request.write(nullptr, 0);

		std::string_view remote = request.remote_addr();
		std::string_view method = request.request_method_string();
		std::string_view uri = request.document_uri();
		std::string_view status = request.http_status();
		m_private->logf(LogLevel::Debug, "%.*s:%d - %.*s - %.*s %.*s\n", int(remote.size()), remote.data(), request.remote_port(), int(status.size()), status.data(), int(method.size()), method.data(), int(uri.size()), uri.data());
	}

	m_private->log(LogLevel::Info, "Thread finished\n");
}


}
