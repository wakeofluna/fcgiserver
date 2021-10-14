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
	using RequestMethodMap = std::map<std::string_view,RequestMethod>;
	static RequestMethodMap rmm({
	    { "GET"sv, RequestMethod::GET },
	    { "HEAD"sv, RequestMethod::HEAD },
	    { "POST"sv, RequestMethod::POST },
	    { "PUT"sv, RequestMethod::PUT },
	    { "DELETE"sv, RequestMethod::DELETE },
	    { "CONNECT"sv, RequestMethod::CONNECT },
	    { "OPTIONS"sv, RequestMethod::OPTIONS },
	    { "TRACE"sv, RequestMethod::TRACE },
	    { "PATCH"sv, RequestMethod::PATCH },
	});

	auto iter = rmm.find(method);
	return iter != rmm.cend() ? iter->second : RequestMethod::OTHER;
}

}

Request::Request(ICgiData & cgidata)
    : m_cgi_data(cgidata)
    , m_headers_sent(false)
{
}

Request::~Request()
{
	if (!m_headers_sent)
	{
		constexpr std::string_view message("Data processor did not return any data"sv);

		set_http_status(500);
		set_content_type("text/plain");
		set_header("Content-Length", message.size());
		write(message);
	}
}

int Request::read(char * buffer, size_t bufsize)
{
	if (bufsize == 0)
		return 0;

	return m_cgi_data.read(reinterpret_cast<uint8_t*>(buffer), bufsize);
}

int Request::write(const char * buffer, size_t bufsize)
{
	send_headers();

	if (bufsize == size_t(-1))
		bufsize = std::strlen(buffer);

	if (bufsize == 0)
		return 0;

	return m_cgi_data.write(reinterpret_cast<const uint8_t*>(buffer), bufsize);
}

int Request::error(const char * buffer, size_t bufsize)
{
	if (bufsize == size_t(-1))
		bufsize = std::strlen(buffer);

	if (bufsize == 0)
		return 0;

	return m_cgi_data.error(reinterpret_cast<const uint8_t*>(buffer), bufsize);
}

int Request::flush()
{
	return m_cgi_data.flush_write();
}

int Request::flush_error()
{
	return m_cgi_data.flush_error();
}

Request::StringViewMap const& Request::env_map() const
{
	if (m_env_map.empty())
	{
		for (const char **envp = m_cgi_data.env(); envp && *envp; ++envp)
		{
			std::string_view line(*envp);

			auto split_pos = line.find('=');
			if (split_pos == std::string_view::npos)
				continue;

			std::string_view key = line.substr(0, split_pos);
			std::string_view val = line.substr(split_pos + 1);
			const_cast<StringViewMap&>(m_env_map).emplace(key, val);
		}
	}
	return m_env_map;
}

std::string_view Request::env(std::string_view const& key) const
{
	StringViewMap const& env = env_map();
	auto iter = env.find(key);
	return iter != env.cend() ? iter->second : std::string_view();
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
	m_headers.emplace("Status"sv, std::to_string(code));
}

void Request::set_content_type(std::string content_type)
{
	m_headers.emplace("Content-Type"sv, std::move(content_type));
}

void Request::set_header(std::string key, std::string value)
{
	m_headers.emplace(std::move(key), std::move(value));
}

void Request::set_header(std::string key, int value)
{
	m_headers.emplace(std::move(key), std::to_string(value));
}

void Request::send_headers()
{
	if (m_headers_sent)
		return;

	if (m_headers.find("Status") == m_headers.cend())
		m_headers.emplace("Status", "200");

	if (m_headers.find("Content-Type") == m_headers.cend())
		m_headers.emplace("Content-Type", "text/html");

	for (auto iter : m_headers)
	{
		m_cgi_data.write(reinterpret_cast<const uint8_t*>(iter.first.c_str()), iter.first.size());
		m_cgi_data.write(reinterpret_cast<const uint8_t*>(": "), 2);
		m_cgi_data.write(reinterpret_cast<const uint8_t*>(iter.second.c_str()), iter.second.size());
		m_cgi_data.write(reinterpret_cast<const uint8_t*>("\r\n"), 2);
	}

	m_cgi_data.write(reinterpret_cast<const uint8_t*>("\r\n"), 2);
	m_headers_sent = true;
}
