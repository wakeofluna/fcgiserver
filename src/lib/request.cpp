#include "i_cgi_data.h"
#include "request.h"
#include <cstring>
#include <charconv>

using namespace fcgiserver;
using namespace std::literals::string_view_literals;

namespace
{

RequestMethod resolve_method(std::string_view const& method)
{
	size_t len = method.size();
	if (len == 3)
	{
		if (method == "GET"sv)
			return RequestMethod::GET;
		else if (method == "PUT"sv)
			return RequestMethod::PUT;
	}
	else if (len == 4)
	{
		if (method == "POST"sv)
			return RequestMethod::POST;
		else if (method == "HEAD"sv)
			return RequestMethod::HEAD;
	}
	else if (len == 5)
	{
		if (method == "PATCH"sv)
			return RequestMethod::PATCH;
		else if (method == "TRACE"sv)
			return RequestMethod::TRACE;
	}
	else if (len == 6)
	{
		if (method == "DELETE"sv)
			return RequestMethod::DELETE;
	}
	else if (len == 7)
	{
		if (method == "CONNECT"sv)
			return RequestMethod::CONNECT;
		else if (method == "OPTIONS"sv)
			return RequestMethod::OPTIONS;
	}
	return RequestMethod::OTHER;
}

}

class fcgiserver::RequestPrivate
{
public:
	RequestPrivate(ICgiData & icd)
	    : cgi_data(icd)
	    , headers_sent(false)
	{}

	ICgiData & cgi_data;
	Request::StringViewMap env_map;
	Request::StringMap headers;
	bool headers_sent;

};

Request::Request(ICgiData & cgidata)
    : m_private(new RequestPrivate(cgidata))
{
}

ICgiData & Request::cgi_data()
{
	return m_private->cgi_data;
}

ICgiData const& Request::cgi_data() const
{
	return m_private->cgi_data;
}

Request::~Request()
{
	if (!m_private->headers_sent)
	{
		constexpr std::string_view message("Data processor did not return any data"sv);

		set_http_status(500);
		set_content_type("text/plain");
		set_header("Content-Length", message.size());
		write(message);
	}
	delete m_private;
}

int Request::read(char * buffer, size_t bufsize)
{
	if (bufsize == 0)
		return 0;

	return m_private->cgi_data.read(reinterpret_cast<uint8_t*>(buffer), bufsize);
}

int Request::write(const char * buffer, size_t bufsize)
{
	send_headers();

	if (bufsize == size_t(-1))
		bufsize = std::strlen(buffer);

	if (bufsize == 0)
		return 0;

	return m_private->cgi_data.write(reinterpret_cast<const uint8_t*>(buffer), bufsize);
}

int Request::error(const char * buffer, size_t bufsize)
{
	if (bufsize == size_t(-1))
		bufsize = std::strlen(buffer);

	if (bufsize == 0)
		return 0;

	return m_private->cgi_data.error(reinterpret_cast<const uint8_t*>(buffer), bufsize);
}

int Request::flush()
{
	return m_private->cgi_data.flush_write();
}

int Request::flush_error()
{
	return m_private->cgi_data.flush_error();
}

Request::StringViewMap const& Request::env_map() const
{
	if (m_private->env_map.empty())
	{
		for (const char **envp = m_private->cgi_data.env(); envp && *envp; ++envp)
		{
			std::string_view line(*envp);

			auto split_pos = line.find('=');
			if (split_pos == std::string_view::npos)
				continue;

			std::string_view key = line.substr(0, split_pos);
			std::string_view val = line.substr(split_pos + 1);
			const_cast<StringViewMap&>(m_private->env_map).emplace(key, val);
		}
	}
	return m_private->env_map;
}

std::string_view Request::env(std::string_view const& key) const
{
	StringViewMap const& env = env_map();
	auto iter = env.find(key);
	return iter != env.cend() ? iter->second : std::string_view();
}

std::string_view Request::header(std::string_view const& key) const
{
	auto iter = m_private->headers.find(std::string(key));
	return iter != m_private->headers.cend() ? iter->second : "200"sv;
}

RequestMethod Request::request_method() const
{
	std::string_view value = request_method_string();
	return value.empty() ? RequestMethod::UNKNOWN : resolve_method(value);
}

Request::StringViewMap Request::query() const
{
	StringViewMap result;

	std::string_view query_str = query_string();
	while (!query_str.empty())
	{
		size_t split = query_str.find('&');

		std::string_view element = query_str.substr(0, split);
		query_str = split == std::string_view::npos ? std::string_view() : query_str.substr(split+1);

		split = element.find('=');
		std::string_view key = element.substr(0, split);
		std::string_view value = split == std::string_view::npos ? std::string_view() : element.substr(split+1);
		result.emplace(key, value);
	}

	return result;
}

int Request::remote_port() const
{
	std::string_view value = remote_port_string();

	unsigned short port;
	auto result = std::from_chars(value.begin(), value.end(), port, 10);

	return result.ec == std::errc() ? port : -1;
}

bool Request::do_not_track() const
{
	std::string_view value = do_not_track_string();
	return value == "1"sv;
}

void Request::set_http_status(uint16_t code)
{
	m_private->headers.emplace("Status"sv, std::to_string(code));
}

void Request::set_content_type(std::string content_type)
{
	m_private->headers.emplace("Content-Type"sv, std::move(content_type));
}

void Request::set_header(std::string key, std::string value)
{
	m_private->headers.emplace(std::move(key), std::move(value));
}

void Request::set_header(std::string key, int value)
{
	m_private->headers.emplace(std::move(key), std::to_string(value));
}

void Request::send_headers()
{
	if (m_private->headers_sent)
		return;

	if (m_private->headers.find("Status") == m_private->headers.cend())
		m_private->headers.emplace("Status", "200");

	if (m_private->headers.find("Content-Type") == m_private->headers.cend())
		m_private->headers.emplace("Content-Type", "text/html");

	for (auto iter : m_private->headers)
	{
		m_private->cgi_data.write(reinterpret_cast<const uint8_t*>(iter.first.c_str()), iter.first.size());
		m_private->cgi_data.write(reinterpret_cast<const uint8_t*>(": "), 2);
		m_private->cgi_data.write(reinterpret_cast<const uint8_t*>(iter.second.c_str()), iter.second.size());
		m_private->cgi_data.write(reinterpret_cast<const uint8_t*>("\r\n"), 2);
	}

	m_private->cgi_data.write(reinterpret_cast<const uint8_t*>("\r\n"), 2);
	m_private->headers_sent = true;
}
