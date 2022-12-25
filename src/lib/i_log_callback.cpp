#include "i_log_callback.h"
#include "line_formatter.h"
#include "request.h"
#include <chrono>

using namespace fcgiserver;

ILogCallback::~ILogCallback() = default;

void ILogCallback::log_request(Request const& request)
{
	LineFormatter lf;
	lf.append(request.remote_addr(), ':', request.remote_port(), " - ", request.http_status(), " - ", request.request_method_string(), ' ', request.document_uri());
	log_message(LogLevel::Debug, lf.buffer());
}

void ILogCallback::now(std::tm *tm)
{
	std::chrono::system_clock::time_point timepoint = std::chrono::system_clock::now();
	time_t now_time = std::chrono::duration_cast<std::chrono::seconds>(timepoint.time_since_epoch()).count();
	gmtime_r(&now_time, tm);
}
