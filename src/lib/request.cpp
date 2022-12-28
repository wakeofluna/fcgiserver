#include "i_cgi_data.h"
#include "request.h"
#include "utils.h"
#include <cstring>
#include <charconv>

using namespace fcgiserver;
using namespace std::literals::string_view_literals;

namespace
{

RequestMethod resolve_method(std::string_view const& method)
{
	Symbol sym = Symbol::maybe(method);

	if (sym == symbols::GET)
		return RequestMethod::GET;
	if (sym == symbols::PUT)
		return RequestMethod::PUT;
	if (sym == symbols::POST)
		return RequestMethod::POST;
	if (sym == symbols::HEAD)
		return RequestMethod::HEAD;
	if (sym == symbols::PATCH)
		return RequestMethod::PATCH;
	if (sym == symbols::TRACE)
		return RequestMethod::TRACE;
	if (sym == symbols::DELETE)
		return RequestMethod::DELETE;
	if (sym == symbols::CONNECT)
		return RequestMethod::CONNECT;
	if (sym == symbols::OPTIONS)
		return RequestMethod::OPTIONS;

	return RequestMethod::Other;
}

template <typename T>
int write_utf32(T & writer, int (T::*out)(uint8_t const*, size_t), char32_t const* buffer, size_t bufsize)
{
	const bool infinite = (bufsize == size_t(-1));

	uint8_t mb_buf[4];
	int written = 0;

	for (; bufsize > 0; ++buffer, --bufsize)
	{
		if (infinite && *buffer == 0)
			break;

		size_t mb_size = utils::utf32_to_8(*buffer, mb_buf);
		if (mb_size == 0)
			return -1;

		int result = (writer.*out)(mb_buf, mb_size);
		if (result < 0)
			return result;

		if (size_t(result) != mb_size)
			return -1;

		written += result;
	}

	return written;
}

}

class fcgiserver::RequestPrivate
{
public:
	RequestPrivate(ICgiData & icd)
	    : cgi_data(icd)
	    , headers_sent(false)
	    , query_parsed(false)
	    , route_parsed(false)
	{}

	ICgiData & cgi_data;
	Request::EnvMap env_map;
	Request::HeaderMap headers;
	Request::QueryParams query;
	Request::Route route;
	Request::Route relative_route;
	ContentEncoding encoding;
	bool headers_sent;
	bool query_parsed;
	bool route_parsed;
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
		set_header(symbols::ContentLength, message.size());
		write(message);
	}
	delete m_private;
}

int Request::read(char * buffer, size_t bufsize)
{
	if (!buffer || bufsize == 0)
		return 0;

	return m_private->cgi_data.read(reinterpret_cast<uint8_t*>(buffer), bufsize);
}

int Request::write(std::string_view const& buffer)
{
	send_headers();
	return m_private->cgi_data.write(reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size());
}

int Request::write(std::u32string_view const& buffer)
{
	send_headers();
	return ::write_utf32(m_private->cgi_data, &ICgiData::write, buffer.data(), buffer.size());
}

int Request::write_html(std::string_view const& buffer)
{
	send_headers();

	utils::convert_state mbstate;
	uint8_t mb_buf[16];
	int written = 0;

	char const* bufdata = buffer.data();
	size_t bufsize = buffer.size();
	for (; bufsize > 0; ++bufdata, --bufsize)
	{
		char32_t glyph = utf8_to_32(*bufdata, mbstate);
		if (!mbstate)
			return -1;

		if (glyph == 0)
			continue;

		int result;
		if (glyph < 0x80)
		{
			uint8_t chr = glyph;
			result = m_private->cgi_data.write(&chr, 1);
		}
		else
		{
			int len = std::snprintf((char*)mb_buf, sizeof(mb_buf), "&#x%x;", glyph);
			result = m_private->cgi_data.write(mb_buf, len);
		}

		if (result < 0)
			return result;

		written += result;
	}

	return written;
}

int Request::write_html(std::u32string_view const& buffer)
{
	send_headers();

	uint8_t mb_buf[16];
	int written = 0;

	char32_t const* bufdata = buffer.data();
	size_t bufsize = buffer.size();
	for (; bufsize > 0; ++bufdata, --bufsize)
	{
		char32_t glyph = *bufdata;

		int result;
		if (glyph < 0x80)
		{
			uint8_t chr = glyph;
			result = m_private->cgi_data.write(&chr, 1);
		}
		else
		{
			int len = std::snprintf((char*)mb_buf, sizeof(mb_buf), "&#x%x;", glyph);
			result = m_private->cgi_data.write(mb_buf, len);
		}

		if (result < 0)
			return result;

		written += result;
	}

	return written;
}

int Request::flush()
{
	return m_private->cgi_data.flush_write();
}

int Request::error(std::string_view const& buffer)
{
	return m_private->cgi_data.error(reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size());
}

int Request::error(std::u32string_view const& buffer)
{
	return ::write_utf32(m_private->cgi_data, &ICgiData::error, buffer.data(), buffer.size());
}

int Request::flush_error()
{
	return m_private->cgi_data.flush_error();
}

Request::EnvMap const& Request::env_map() const
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
			m_private->env_map.emplace(Symbol(key), val);
		}
	}
	return m_private->env_map;
}

const Request::HeaderMap & Request::headers() const
{
	return m_private->headers;
}

std::string_view Request::env(Symbol key) const
{
	EnvMap const& env = env_map();
	auto iter = env.find(key);
	return iter != env.cend() ? iter->second : std::string_view();
}

std::string_view Request::header(Symbol key) const
{
	auto iter = m_private->headers.find(key);
	return iter != m_private->headers.cend() ? iter->second : std::string_view();
}

RequestMethod Request::request_method() const
{
	std::string_view value = request_method_string();
	return value.empty() ? RequestMethod::Unknown : resolve_method(value);
}

Request::Route const& Request::full_route() const
{
	if (!m_private->route_parsed)
	{
		m_private->route.reserve(16);

		std::string_view uri = document_uri();
		while (!uri.empty())
		{
			size_t split = uri.find('/');

			std::string_view element = uri.substr(0, split);
			uri = (split == std::string_view::npos) ? std::string_view() : uri.substr(split+1);

			if (!element.empty())
				m_private->route.emplace_back(element);
		}

		m_private->route_parsed = true;
		m_private->relative_route = m_private->route;
	}

	return m_private->route;
}

Request::Route const& Request::relative_route() const
{
	if (!m_private->route_parsed)
		full_route();
	return m_private->relative_route;
}

void Request::swap_relative_route(Request::Route & route)
{
	m_private->relative_route.swap(route);
}

Request::QueryParams const& Request::query() const
{
	if (!m_private->query_parsed)
	{
		m_private->query.reserve(16);

		std::string_view query_str = query_string();
		while (!query_str.empty())
		{
			size_t split = query_str.find('&');

			std::string_view element = query_str.substr(0, split);
			query_str = split == std::string_view::npos ? std::string_view() : query_str.substr(split+1);

			split = element.find('=');
			std::string_view key = element.substr(0, split);
			std::string_view value = split == std::string_view::npos ? std::string_view() : element.substr(split+1);
			m_private->query.emplace_back(key, value);
		}

		m_private->query_parsed = true;
	}

	return m_private->query;
}

std::pair<bool,std::string_view> Request::query(const std::string_view & key) const
{
	Request::QueryParams const& params = query();

	for (auto const& item : params)
	{
		if (item.first == key)
			return std::make_pair(true, item.second);
	}

	return std::make_pair(false, std::string_view());
}

std::pair<bool,std::u32string> Request::query_decode(const std::string_view & value)
{
	std::u32string result;
	result.reserve(value.size());

	utils::convert_state mbstate;

	for (size_t idx = 0; idx < value.size(); ++idx)
	{
		char32_t glyph;

		char c = value[idx];
		if (c == '%' && idx + 2 < value.size())
		{
			auto chr = utils::unhex(value.data() + idx + 1);
			if (chr.first)
			{
				glyph = utils::utf8_to_32(chr.second, mbstate);
				idx += 2;
			}
			else
			{
				glyph = utils::utf8_to_32(c, mbstate);
			}
		}
		else
		{
			glyph = utf8_to_32(c, mbstate);
		}

		if (!mbstate)
			return {false,std::u32string()};

		if (glyph != 0)
			result.push_back(glyph);
	}

	return {true,std::move(result)};
}

std::pair<bool,std::u32string> Request::utf8_decode(std::string_view const& value)
{
	std::u32string result;
	result.reserve(value.size());

	utils::convert_state mbstate;

	for (size_t idx = 0; idx < value.size(); ++idx)
	{
		char32_t glyph = utils::utf8_to_32(value[idx], mbstate);

		if (!mbstate)
			return {false,std::u32string()};

		if (glyph != 0)
			result.push_back(glyph);
	}

	return {true,std::move(result)};
}

std::pair<bool,std::string> Request::utf8_encode(std::u32string_view const& value)
{
	struct Writer
	{
		inline Writer(std::string & _target) : target(_target) {}
		std::string & target;

		int write(uint8_t const* buffer, size_t bufsize)
		{
			target.append((char const*)buffer, bufsize);
			return bufsize;
		}
	};

	std::string result;
	result.reserve(value.size() + 32);

	Writer writer(result);
	int written = write_utf32(writer, &Writer::write, value.data(), value.size());

	if (written < 0)
		return {false, std::string()};

	return {true, std::move(result)};
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
	m_private->headers.emplace(symbols::Status, std::to_string(code));
}

void Request::set_content_type(std::string content_type)
{
	m_private->headers.emplace(symbols::ContentType, std::move(content_type));

	// Some very crude autodetection
	std::string_view ct = content_type;
	if (ct == "text/html"sv)
		m_private->encoding = ContentEncoding::HTML;
	else if (ct.substr(0, 5) == "text/"sv)
		m_private->encoding = ContentEncoding::UTF8;
	else
		m_private->encoding = ContentEncoding::Verbatim;
}

void Request::set_content_type(std::string content_type, ContentEncoding encoding)
{
	m_private->headers.emplace(symbols::ContentType, std::move(content_type));
	m_private->encoding = encoding;
}

void Request::set_header(Symbol key, std::string value)
{
	m_private->headers.emplace(key, std::move(value));
}

void Request::set_header(Symbol key, int value)
{
	m_private->headers.emplace(key, std::to_string(value));
}

ContentEncoding Request::encoding() const
{
	return m_private->encoding;
}

void Request::set_encoding(ContentEncoding encoding)
{
	m_private->encoding = encoding;
}

void Request::send_headers()
{
	if (m_private->headers_sent)
		return;

	if (m_private->headers.find(symbols::Status) == m_private->headers.cend())
		m_private->headers.emplace(symbols::Status, "200");

	for (auto iter : m_private->headers)
	{
		std::string_view key = iter.first.to_string_view();
		m_private->cgi_data.write(reinterpret_cast<const uint8_t*>(key.data()), key.size());
		m_private->cgi_data.write(reinterpret_cast<const uint8_t*>(": "), 2);
		m_private->cgi_data.write(reinterpret_cast<const uint8_t*>(iter.second.c_str()), iter.second.size());
		m_private->cgi_data.write(reinterpret_cast<const uint8_t*>("\r\n"), 2);
	}

	m_private->cgi_data.write(reinterpret_cast<const uint8_t*>("\r\n"), 2);
	m_private->headers_sent = true;
}
