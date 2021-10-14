#include "fast_cgi_data.h"
#include "request.h"

#include <errno.h>
#include <signal.h>
#include <sys/stat.h>

#include <cstdlib>
#include <cstdio>
#include <thread>
#include <cstring>
#include <unistd.h>

#include <fcgiapp.h>

namespace
{

void sigusr1(int signum, siginfo_t *info, void *ucontext)
{
}

}

void threadfunc(int sockfd)
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

	result = FCGX_InitRequest(&fcgx_request, sockfd, 0);
	if (result != 0)
	{
		printf("Error %d on FCGX_InitRequest! (%s)\n", result, strerror(-result));
		return;
	}

	while (true)
	{
		result = FCGX_Accept_r(&fcgx_request);
		if (result != 0)
		{
			if (result != -9999) // Regular Termination
				printf("Error %d on FCGX_Accept_r! (%s)\n", result, strerror(-result));
			return;
		}

		fcgiserver::FastCgiData fcgi_data(fcgx_request);
		fcgiserver::Request request(fcgi_data);

		request.write("<html><body>\n");
		request.write("<h1>Hello world!</h1>\n");

		request.write("<table>\n");
		request.write("<thead><th>Key</th><th>Value</th></thead>\n");
		request.write("<tbody>\n");
		auto const& env_map = request.env_map();
		for (auto & entry : env_map)
		{
			request.write("<tr><td>");
			request.write(entry.first);
			request.write("</td><td>");
			request.write(entry.second);
			request.write("</td></tr>");
		}
		request.write("</tbody>\n");
		request.write("</table>\n");

		request.write("</body></html>\n");
		printf("Finished request..\n");
	}

	printf("Thread finished\n");
}


int main(int argc, char **argv)
{
	const int nr_threads = 4;
	int result;
	int sockfd = -1;

	result = FCGX_Init();
	if (result != 0)
	{
		printf("Error %d on FCGX_Init!\n", result);
		return EXIT_FAILURE;
	}

	if (FCGX_IsCGI())
	{
		// Running standalone, need to create our own socket
		umask(0);

		sockfd = FCGX_OpenSocket("/tmp/fcgiserver.sock", nr_threads);
		if (sockfd == -1)
		{
			printf("Some socket error %d\n", errno);
			return EXIT_FAILURE;
		}
	}
	else
	{
		// Running in webserver context, accept it as reality
		sockfd = 0;
	}

	printf("Running %d threads on socket %d!\n", nr_threads, sockfd);

	std::thread threads[nr_threads];
	for (int i = 0; i < nr_threads; ++i)
		threads[i] = std::thread(&threadfunc, sockfd);

	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGTERM);
	sigaddset(&sigset, SIGHUP);
	pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

	int signum = 0;
	sigwait(&sigset, &signum);
	pthread_sigmask(SIG_UNBLOCK, &sigset, nullptr);

	printf("Initiating shutdown! (signal:%d)\n", signum);
	FCGX_ShutdownPending();

	if (sockfd != 0)
	{
		::close(sockfd);
		::unlink("/tmp/fcgiserver.sock");
	}

	for (int i = 0; i < nr_threads; ++i)
		if (threads[i].joinable())
			pthread_kill(threads[i].native_handle(), SIGUSR1);

	for (int i = 0; i < nr_threads; ++i)
		if (threads[i].joinable())
			threads[i].join();

	printf("All done!\n");
	return EXIT_SUCCESS;
}
