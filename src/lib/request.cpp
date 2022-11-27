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

std::pair<bool,uint8_t> unhex(char const* ch)
{
	uint8_t v = 0;

	if (ch[0] >= '0' && ch[0] <= '9')
		v += (ch[0] - '0');
	else if (ch[0] >= 'a' && ch[0] <= 'f')
		v += (ch[0] - 'a' + 10);
	else if (ch[0] >= 'A' && ch[0] <= 'F')
		v += (ch[0] - 'A' + 10);
	else
		return {false, 0U};

	v <<= 4;

	if (ch[1] >= '0' && ch[1] <= '9')
		v += (ch[1] - '0');
	else if (ch[1] >= 'a' && ch[1] <= 'f')
		v += (ch[1] - 'a' + 10);
	else if (ch[1] >= 'A' && ch[1] <= 'F')
		v += (ch[1] - 'A' + 10);
	else
		return {false, 0U};

	return {true, v};
}

struct convert_state
{
	uint32_t glyph = 0;
	uint8_t remaining = 0;
	bool valid = false;
	inline operator bool() const { return valid; }
};

char32_t utf8_to_32(uint8_t next, convert_state & state)
{
	if (state.remaining > 0)
	{
		if (state.valid)
		{
			if ((next & 0xc0) == 0x80)
			{
				state.glyph <<= 6;
				state.glyph |= (next & 0x3f);
			}
			else
			{
				state.valid = false;
			}
		}
		--state.remaining;
	}
	else
	{
		if ((next & 0x80) == 0x00)
		{
			state.valid = true;
			state.glyph = next;
		}
		else if ((next & 0xe0) == 0xc0)
		{
			state.valid = true;
			state.remaining = 1;
			state.glyph = (next & 0x1f);
		}
		else if ((next & 0xf0) == 0xe0)
		{
			state.valid = true;
			state.remaining = 2;
			state.glyph = (next & 0x0f);
		}
		else if ((next & 0xf8) == 0xf0)
		{
			state.valid = true;
			state.remaining = 3;
			state.glyph = (next & 0x07);
		}
		else
		{
			state.valid = false;
		}
	}

	return (state.valid && state.remaining == 0) ? state.glyph : 0;
}

size_t utf32_to_8(char32_t glyph, uint8_t * buf)
{
	if (glyph < 0x80)
	{
		buf[0] = glyph;
		return 1;
	}
	else if (glyph < 0x800)
	{
		buf[0] = 0xc0 + (glyph >> 6);
		buf[1] = 0x80 + (glyph & 0x3f);
		return 2;
	}
	else if (glyph < 0x10000)
	{
		buf[0] = 0xe0 + (glyph >> 12);
		buf[1] = 0x80 + ((glyph >> 6) & 0x3f);
		buf[2] = 0x80 + (glyph & 0x3f);
		return 3;
	}
	else if (glyph < 0x110000)
	{
		buf[0] = 0xf0 + (glyph >> 18);
		buf[1] = 0x80 + ((glyph >> 12) & 0x3f);
		buf[2] = 0x80 + ((glyph >> 6) & 0x3f);
		buf[3] = 0x80 + (glyph & 0x3f);
		return 4;
	}
	else
	{
		return 0;
	}
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

		size_t mb_size = utf32_to_8(*buffer, mb_buf);
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

int Request::write(const char32_t * buffer, size_t bufsize)
{
	send_headers();

	return ::write_utf32(m_private->cgi_data, &ICgiData::write, buffer, bufsize);
}

int Request::write_html(const char * buffer, size_t bufsize)
{
	send_headers();

	const bool infinite = (bufsize == size_t(-1));

	convert_state mbstate;
	uint8_t mb_buf[16];
	int written = 0;

	for (; bufsize > 0; ++buffer, --bufsize)
	{
		if (infinite && *buffer == 0)
			break;

		char32_t glyph = utf8_to_32(*buffer, mbstate);
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

int Request::write_html(const char32_t * buffer, size_t bufsize)
{
	send_headers();

	const bool infinite = (bufsize == size_t(-1));

	uint8_t mb_buf[16];
	int written = 0;

	for (; bufsize > 0; ++buffer, --bufsize)
	{
		char32_t glyph = *buffer;
		if (infinite && glyph == 0)
			break;

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

int Request::error(const char * buffer, size_t bufsize)
{
	if (bufsize == size_t(-1))
		bufsize = std::strlen(buffer);

	if (bufsize == 0)
		return 0;

	return m_private->cgi_data.error(reinterpret_cast<const uint8_t*>(buffer), bufsize);
}

int Request::error(const char32_t * buffer, size_t bufsize)
{
	return ::write_utf32(m_private->cgi_data, &ICgiData::error, buffer, bufsize);
}

int Request::flush()
{
	return m_private->cgi_data.flush_write();
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

	convert_state mbstate;

	for (size_t idx = 0; idx < value.size(); ++idx)
	{
		char32_t glyph;

		char c = value[idx];
		if (c == '%' && idx + 2 < value.size())
		{
			auto chr = unhex(value.data() + idx + 1);
			if (chr.first)
			{
				glyph = utf8_to_32(chr.second, mbstate);
				idx += 2;
			}
			else
			{
				glyph = utf8_to_32(c, mbstate);
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

	convert_state mbstate;

	for (size_t idx = 0; idx < value.size(); ++idx)
	{
		char32_t glyph = utf8_to_32(value[idx], mbstate);

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
}

void Request::set_header(Symbol key, std::string value)
{
	m_private->headers.emplace(key, std::move(value));
}

void Request::set_header(Symbol key, int value)
{
	m_private->headers.emplace(key, std::to_string(value));
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
