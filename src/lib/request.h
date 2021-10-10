#ifndef FCGISERVER_REQUEST_H
#define FCGISERVER_REQUEST_H

#include "request_method.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string_view>

namespace fcgiserver
{

class ICgiData;

class Request
{
public:
	Request(ICgiData & cgidata);
	~Request();

	inline ICgiData      & cgi_data()       { return m_cgi_data; }
	inline ICgiData const& cgi_data() const { return m_cgi_data; }

	int read(char * buffer, size_t bufsize);
	int write(const char * buffer, size_t bufsize = size_t(-1));
	int error(const char * buffer, size_t bufsize = size_t(-1));
	inline int write(std::string_view const& buf, size_t max_len = size_t(-1)) { return write(buf.data(), std::min(buf.size(), max_len)); }
	inline int error(std::string_view const& buf, size_t max_len = size_t(-1)) { return error(buf.data(), std::min(buf.size(), max_len)); }
	int flush();
	int flush_error();

	using StringMap = std::map<std::string_view,std::string_view>;
	StringMap const& env_map() const;

	std::string_view env(std::string_view const& key) const;
	inline std::string_view request_method_string() const { return env("REQUEST_METHOD"); }
	inline std::string_view query_string() const { return env("QUERY_STRING"); }
	inline std::string_view script_name() const { return env("SCRIPT_NAME"); }
	inline std::string_view document_uri() const { return env("DOCUMENT_URI"); }
	inline std::string_view request_scheme() const { return env("REQUEST_SCHEME"); }
	inline std::string_view remote_addr() const { return env("REMOTE_ADDR"); }
	inline std::string_view remote_port_string() const { return env("REMOTE_PORT"); }
	inline std::string_view user_agent() const { return env("HTTP_USER_AGENT"); }
	inline std::string_view do_not_track_string() const { return env("HTTP_DNT"); }

	RequestMethod request_method() const;
	StringMap query() const;
	int remote_port() const;
	bool do_not_track() const;

protected:
	ICgiData & m_cgi_data;
	StringMap m_env_map;
};

} // namespace fcgiserver

#endif // FCGISERVER_ICGIDATA_H
