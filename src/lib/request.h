#ifndef FCGISERVER_REQUEST_H
#define FCGISERVER_REQUEST_H

#include "request_method.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string_view>
#include <string>

using namespace std::literals::string_view_literals;

namespace fcgiserver
{

class ICgiData;

class Request
{
public:
	using StringViewMap = std::map<std::string_view,std::string_view>;
	using StringMap = std::map<std::string,std::string>;

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

	StringViewMap const& env_map() const;

	std::string_view env(std::string_view const& key) const;
	std::string_view header(std::string_view const& key) const;
	inline std::string_view request_method_string() const { return env("REQUEST_METHOD"sv); }
	inline std::string_view query_string() const { return env("QUERY_STRING"sv); }
	inline std::string_view script_name() const { return env("SCRIPT_NAME"sv); }
	inline std::string_view document_uri() const { return env("DOCUMENT_URI"sv); }
	inline std::string_view request_scheme() const { return env("REQUEST_SCHEME"sv); }
	inline std::string_view remote_addr() const { return env("REMOTE_ADDR"sv); }
	inline std::string_view remote_port_string() const { return env("REMOTE_PORT"sv); }
	inline std::string_view user_agent() const { return env("HTTP_USER_AGENT"sv); }
	inline std::string_view do_not_track_string() const { return env("HTTP_DNT"sv); }
	inline std::string_view http_status() const { return header("Status"sv); }

	RequestMethod request_method() const;
	StringViewMap query() const;
	int remote_port() const;
	bool do_not_track() const;

	void set_http_status(uint16_t code);
	void set_content_type(std::string content_type);
	void set_header(std::string key, std::string value);
	void set_header(std::string key, int value);

protected:
	void send_headers();

	ICgiData & m_cgi_data;
	StringViewMap m_env_map;
	StringMap m_headers;
	bool m_headers_sent;
};

} // namespace fcgiserver

#endif // FCGISERVER_ICGIDATA_H
