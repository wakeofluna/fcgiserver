#ifndef FCGISERVER_REQUEST_H
#define FCGISERVER_REQUEST_H

#include "fcgiserver_defs.h"
#include "generic_formatter.h"
#include "request_method.h"
#include "symbol.h"
#include "symbols.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>
#include <string_view>
#include <string>

using namespace std::literals::string_view_literals;

namespace fcgiserver
{

class ICgiData;
class RequestPrivate;
class Request;
class RequestStream;

enum class ContentEncoding
{
	Verbatim,
	UTF8,
	HTML,
};

class DLL_PUBLIC Request
{
public:
	using QueryParams = std::vector<std::pair<std::string_view,std::string_view>>;
	using EnvMap = std::map<Symbol,std::string_view>;
	using HeaderMap = std::unordered_map<Symbol,std::string>;
	using Route = std::vector<std::string_view>;

	Request(ICgiData & cgidata);
	~Request();

	Request(Request && other) = delete;
	Request(Request const& other) = delete;

	ICgiData      & cgi_data();
	ICgiData const& cgi_data() const;

	int read(char * buffer, size_t bufsize);

	RequestStream write_stream();
	int write(std::string_view const& buf);
	int flush_write();

	RequestStream error_stream();
	int error(std::string_view const& buf);
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

	QueryParams const& query() const;
	std::pair<bool,std::string_view> query(std::string_view const& key) const;
	static std::pair<bool,std::string> query_decode(std::string_view const& value);
	static std::pair<bool,std::u32string> utf8_decode(std::string_view const& value);
	static std::pair<bool,std::string> utf8_encode(std::u32string_view const& value);

	RequestMethod request_method() const;
	Route const& full_route() const;
	Route const& relative_route() const;
	void swap_relative_route(Route & route);
	int remote_port() const;
	bool do_not_track() const;

	void set_http_status(uint16_t code);
	void set_content_type(std::string content_type);
	void set_content_type(std::string content_type, ContentEncoding encoding);
	void set_header(Symbol key, std::string value);
	void set_header(Symbol key, int value);

	ContentEncoding encoding() const;
	void set_encoding(ContentEncoding encoding);

	void send_headers();

protected:
	RequestPrivate * m_private;
};

struct DLL_PUBLIC HTMLContent
{
	explicit constexpr HTMLContent(std::string_view s) : content(s) {}
	std::string_view content;
};

class DLL_PUBLIC RequestStream : public GenericFormatter
{
public:
	RequestStream(Request & request, int (Request::*channel)(std::string_view const&), GenericFormat format);
	RequestStream(Request const& other) = delete;
	RequestStream(Request && other) = delete;
	~RequestStream() = default;

	RequestStream & operator<< (HTMLContent const& value);

	template <typename T>
	RequestStream & operator<< (T const& value)
	{
		GenericFormatter::operator<<(value);
		return *this;
	}

protected:
	Request & m_request;
	int (Request::*m_channel)(std::string_view const& s);

	void real_append(std::string_view const& s) override;
};

} // namespace fcgiserver

#endif // FCGISERVER_ICGIDATA_H
