#ifndef FCGISERVER_REQUEST_H
#define FCGISERVER_REQUEST_H

#include "fcgiserver_defs.h"
#include "request_method.h"
#include "symbol.h"
#include "symbols.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <string_view>
#include <string>

using namespace std::literals::string_view_literals;

namespace fcgiserver
{

class ICgiData;
class RequestPrivate;

class DLL_PUBLIC Request
{
public:
	using StringViewMap = std::map<std::string_view,std::string_view>;
	using EnvMap = std::map<Symbol,std::string_view>;
	using HeaderMap = std::unordered_map<Symbol,std::string>;

	Request(ICgiData & cgidata);
	~Request();

	Request(Request && other) = delete;
	Request(Request const& other) = delete;

	ICgiData      & cgi_data();
	ICgiData const& cgi_data() const;

	int read(char * buffer, size_t bufsize);
	int write(const char * buffer, size_t bufsize = size_t(-1));
	int error(const char * buffer, size_t bufsize = size_t(-1));
	inline int write(std::string_view const& buf, size_t max_len = size_t(-1)) { return write(buf.data(), std::min(buf.size(), max_len)); }
	inline int error(std::string_view const& buf, size_t max_len = size_t(-1)) { return error(buf.data(), std::min(buf.size(), max_len)); }
	int flush();
	int flush_error();

	EnvMap const& env_map() const;
	std::string_view env(Symbol symbol) const;
	inline std::string_view request_content_type() const { return env(symbols::CONTENT_TYPE); }
	inline std::string_view request_content_length() const { return env(symbols::CONTENT_LENGTH); }
	inline std::string_view request_method_string() const { return env(symbols::REQUEST_METHOD); }
	inline std::string_view request_scheme() const { return env(symbols::REQUEST_SCHEME); }
	inline std::string_view query_string() const { return env(symbols::QUERY_STRING); }
	inline std::string_view script_name() const { return env(symbols::SCRIPT_NAME); }
	inline std::string_view document_uri() const { return env(symbols::DOCUMENT_URI); }
	inline std::string_view path_info() const { return env(symbols::PATH_INFO); }
	inline std::string_view remote_addr() const { return env(symbols::REMOTE_ADDR); }
	inline std::string_view remote_port_string() const { return env(symbols::REMOTE_PORT); }
	inline std::string_view user_agent() const { return env(symbols::HTTP_USER_AGENT); }
	inline std::string_view do_not_track_string() const { return env(symbols::HTTP_DNT); }

	HeaderMap const& headers() const;
	std::string_view header(Symbol symbol) const;
	inline std::string_view http_status() const { return header(symbols::Status); }

	RequestMethod request_method() const;
	StringViewMap query() const;
	int remote_port() const;
	bool do_not_track() const;

	void set_http_status(uint16_t code);
	void set_content_type(std::string content_type);
	void set_header(Symbol key, std::string value);
	void set_header(Symbol key, int value);

protected:
	void send_headers();

	RequestPrivate * m_private;
};

} // namespace fcgiserver

#endif // FCGISERVER_ICGIDATA_H
