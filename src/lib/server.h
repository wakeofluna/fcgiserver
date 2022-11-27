#ifndef FCGISERVER_SERVER_H
#define FCGISERVER_SERVER_H

#include "fcgiserver_defs.h"
#include "logger.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace fcgiserver
{

class Request;
class IRouter;
class ServerPrivate;

class DLL_PUBLIC Server
{
public:
	Server();
	~Server();

	void set_router(std::shared_ptr<IRouter> router);
	void set_log_callback(LogCallback && callback);
	void log(LogLevel lvl, const char * msg) const;

	bool initialize(std::string socket_path);
	bool add_threads(size_t count);
	void kill_threads();
	void finalize();

	int wait_for_terminate_signal() const;

private:
	static void run_thread_function(Server *);
	void thread_function();

	ServerPrivate * m_private;
};

}

#endif // FCGISERVER_SERVER_H
