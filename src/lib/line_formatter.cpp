#include "line_formatter.h"
#include <charconv>
#include <cstdio>
#include <cstring>

using namespace fcgiserver;
using namespace std::literals::string_view_literals;


LineFormatter::LineFormatter()
    : GenericFormatter(GenericFormat::UTF8)
{
	m_buffer.reserve(128);
}

LineFormatter::LineFormatter(std::string && buffer)
    : GenericFormatter(GenericFormat::UTF8)
    , m_buffer(std::move(buffer))
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

void LineFormatter::real_append(const std::string_view & s)
{
	m_buffer.append(s);
}

