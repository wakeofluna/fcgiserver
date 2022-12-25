#include "line_formatter.h"
#include <charconv>
#include <cstdio>
#include <cstring>

using namespace fcgiserver;
using namespace std::literals::string_view_literals;


LineFormatter::LineFormatter()
{
	m_buffer.reserve(128);
}

LineFormatter::LineFormatter(std::string && buffer)
    : m_buffer(std::move(buffer))
{
	if (m_buffer.capacity() == 0)
		m_buffer.reserve(128);
}

LineFormatter::~LineFormatter()
{
}

void LineFormatter::clear()
{
	m_buffer.clear();
}

bool LineFormatter::empty() const
{
	return m_buffer.empty();
}

LineFormatter & LineFormatter::printf(char const* fmt, ...)
{
	std::va_list vl;
	va_start(vl, fmt);
	vprintf(fmt, vl);
	va_end(vl);
	return *this;
}

LineFormatter & LineFormatter::vprintf(char const* fmt, std::va_list vl)
{
	char tmp[128];
	auto retval = std::vsnprintf(tmp, sizeof(tmp), fmt, vl);

	if (retval < 0)
	{
		m_buffer.append(fmt);
	}
	else if (retval	== 0)
	{
	}
	else if (size_t(retval) <= sizeof(tmp))
	{
		m_buffer.append(tmp, retval);
	}
	else
	{
		auto start_offset = m_buffer.size();
		m_buffer.append(size_t(retval) + 1, char(0));
		std::vsnprintf(m_buffer.data() + start_offset, retval + 1, fmt, vl);
	}
	return *this;
}

LineFormatter & LineFormatter::operator<< (bool b)
{
	m_buffer.append(b ? "true"sv : "false"sv);
	return *this;
}

LineFormatter & LineFormatter::operator<< (char c)
{
	m_buffer.push_back(c);
	return *this;
}

LineFormatter & LineFormatter::operator<< (uint8_t i)
{
	char tmp[8];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	m_buffer.append(tmp, result.ptr);
	return *this;
}

LineFormatter & LineFormatter::operator<< (int8_t i)
{
	char tmp[8];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	m_buffer.append(tmp, result.ptr);
	return *this;
}

LineFormatter & LineFormatter::operator<< (uint16_t i)
{
	char tmp[8];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	m_buffer.append(tmp, result.ptr);
	return *this;
}

LineFormatter & LineFormatter::operator<< (int16_t i)
{
	char tmp[8];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	m_buffer.append(tmp, result.ptr);
	return *this;
}

LineFormatter & LineFormatter::operator<< (uint32_t i)
{
	char tmp[12];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	m_buffer.append(tmp, result.ptr);
	return *this;
}

LineFormatter & LineFormatter::operator<< (int32_t i)
{
	char tmp[12];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	m_buffer.append(tmp, result.ptr);
	return *this;
}

LineFormatter & LineFormatter::operator<< (uint64_t i)
{
	char tmp[24];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	m_buffer.append(tmp, result.ptr);
	return *this;
}

LineFormatter & LineFormatter::operator<< (int64_t i)
{
	char tmp[24];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	m_buffer.append(tmp, result.ptr);
	return *this;
}

LineFormatter & LineFormatter::operator<< (float f)
{
	char tmp[32];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), f);
	m_buffer.append(tmp, result.ptr);
	return *this;
}

LineFormatter & LineFormatter::operator<< (double d)
{
	char tmp[32];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), d);
	m_buffer.append(tmp, result.ptr);
	return *this;
}

LineFormatter & LineFormatter::operator<< (const char * s)
{
	m_buffer.append(s, std::strlen(s));
	return *this;
}

LineFormatter & LineFormatter::operator<< (std::string_view const& s)
{
	m_buffer.append(s);
	return *this;
}

LineFormatter & LineFormatter::operator<< (std::string const& s)
{
	m_buffer.append(s);
	return *this;
}

LineFormatter & LineFormatter::operator<< (Symbol s)
{
	m_buffer.append(s.to_string_view());
	return *this;
}

