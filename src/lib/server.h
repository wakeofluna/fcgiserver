#ifndef FCGISERVER_SERVER_H
#define FCGISERVER_SERVER_H

#include "fcgiserver_defs.h"
#include "logger.h"
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace fcgiserver
{

class Request;
class IRouter;
class ServerPrivate;
class UserContext;

class DLL_PUBLIC Server
{
public:
	Server();
	~Server();

	Logger & logger();

	void set_router(std::shared_ptr<IRouter> router);
	void set_global_context(std::shared_ptr<UserContext> const& new_context);
	void set_thread_context(std::function<UserContext*(std::shared_ptr<UserContext> const&)> && create_context_function);
	void set_thread_context_tick_interval(std::chrono::seconds duration);

	bool initialize(std::string socket_path);
	bool add_threads(size_t count);
	void kill_threads();
	void finalize();

	int wait_for_terminate_signal() const;

private:
	static void install_thread_signal_handlers();

	static void run_thread_function(Server *, size_t);
	void thread_function(size_t);

	static void run_thread_tick_function(Server *);
	void thread_tick_function();

	ServerPrivate * m_private;
};

}

#endif // FCGISERVER_SERVER_H
