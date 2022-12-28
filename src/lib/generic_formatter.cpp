#include "generic_formatter.h"
#include "utils.h"
#include <charconv>
#include <cstring>
#include <memory>

using namespace fcgiserver;
using namespace std::literals::string_view_literals;

GenericFormatter::GenericFormatter(GenericFormat format)
    : m_generic_format(format)
{
}

GenericFormatter & GenericFormatter::printf(char const* fmt, ...)
{
	std::va_list vl;
	va_start(vl, fmt);
	vprintf(fmt, vl);
	va_end(vl);
	return *this;
}

GenericFormatter & GenericFormatter::vprintf(char const* fmt, std::va_list vl)
{
	char tmp[128];

	std::va_list vl_copy;
	va_copy(vl_copy, vl);
	auto retval = std::vsnprintf(tmp, sizeof(tmp), fmt, vl_copy);
	va_end(vl_copy);

	if (retval < 0)
	{
		operator<< (std::string_view(fmt));
	}
	else if (retval	== 0)
	{
	}
	else if (size_t(retval) <= sizeof(tmp))
	{
		operator<< (std::string_view(tmp, retval));
	}
	else
	{
		std::unique_ptr<char[]> large_tmp(new char[retval + 1]);
		std::vsnprintf(large_tmp.get(), retval + 1, fmt, vl);
		operator<< (std::string_view(large_tmp.get(), retval));
	}
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (bool b)
{
	real_append(b ? "true"sv : "false"sv);
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (char c)
{
	real_append(std::string_view(&c, 1));
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (uint8_t i)
{
	char tmp[8];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	real_append(std::string_view(tmp, result.ptr - tmp));
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (int8_t i)
{
	char tmp[8];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	real_append(std::string_view(tmp, result.ptr - tmp));
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (uint16_t i)
{
	char tmp[8];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	real_append(std::string_view(tmp, result.ptr - tmp));
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (int16_t i)
{
	char tmp[8];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	real_append(std::string_view(tmp, result.ptr - tmp));
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (uint32_t i)
{
	char tmp[12];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	real_append(std::string_view(tmp, result.ptr - tmp));
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (int32_t i)
{
	char tmp[12];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	real_append(std::string_view(tmp, result.ptr - tmp));
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (uint64_t i)
{
	char tmp[24];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	real_append(std::string_view(tmp, result.ptr - tmp));
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (int64_t i)
{
	char tmp[24];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), i);
	real_append(std::string_view(tmp, result.ptr - tmp));
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (float f)
{
	char tmp[32];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), f);
	real_append(std::string_view(tmp, result.ptr - tmp));
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (double d)
{
	char tmp[32];
	auto result = std::to_chars(tmp, tmp + sizeof(tmp), d);
	real_append(std::string_view(tmp, result.ptr - tmp));
	return *this;
}

GenericFormatter & GenericFormatter::operator<< (const char * s)
{
	return operator<< (std::string_view(s));
}

GenericFormatter & GenericFormatter::operator<< (std::string_view const& s)
{
	if (m_generic_format == GenericFormat::Verbatim)
	{
		real_append(s);
	}
	else
	{
		char tmp[128];

		utils::convert_state mbstate;
		size_t offset = 0;

		for (char c : s)
		{
			char32_t glyph = utils::utf8_to_32(c, mbstate);

			if (glyph == 0)
				continue;

			if (m_generic_format == GenericFormat::UTF8)
			{
				size_t len = utils::utf32_to_8(glyph, reinterpret_cast<uint8_t*>(tmp + offset));
				offset += len;
			}
			else if (glyph < 0x80)
			{
				if (c == '<' && m_generic_format == GenericFormat::HTMLContent)
				{
					std::memcpy(tmp + offset, "&lt;", 4);
					offset += 4;
				}
				else if (c == '>' && m_generic_format == GenericFormat::HTMLContent)
				{
					std::memcpy(tmp + offset, "&rt;", 4);
					offset += 4;
				}
				else if (c == '&')
				{
					std::memcpy(tmp + offset, "&amp;", 5);
					offset += 5;
				}
				else
				{
					tmp[offset] = c;
					offset += 1;
				}
			}
			else
			{
				int len = std::snprintf(tmp + offset, sizeof(tmp) - offset, "&#x%x;", glyph);
				offset += len;
			}

			if (offset >= 100)
			{
				real_append(std::string_view(tmp, offset));
				offset = 0;
			}
		}

		if (offset > 0)
			real_append(std::string_view(tmp, offset));
	}

	return *this;
}

GenericFormatter & GenericFormatter::operator<< (std::string const& s)
{
	return operator<< (std::string_view(s));
}

GenericFormatter & GenericFormatter::operator<< (std::u32string_view const& s)
{
	if (m_generic_format == GenericFormat::Verbatim)
	{
		real_append(std::string_view(reinterpret_cast<const char*>(s.data()), s.size() * sizeof(std::u32string_view::value_type)));
	}
	else
	{
		char tmp[128];
		size_t offset = 0;

		for (char32_t glyph : s)
		{
			if (m_generic_format == GenericFormat::UTF8)
			{
				size_t len = utils::utf32_to_8(glyph, reinterpret_cast<uint8_t*>(tmp + offset));
				offset += len;
			}
			else if (glyph < 0x80)
			{
				if (glyph == U'<' && m_generic_format == GenericFormat::HTMLContent)
				{
					std::memcpy(tmp + offset, "&lt;", 4);
					offset += 4;
				}
				else if (glyph == U'>' && m_generic_format == GenericFormat::HTMLContent)
				{
					std::memcpy(tmp + offset, "&rt;", 4);
					offset += 4;
				}
				else if (glyph == U'&')
				{
					std::memcpy(tmp + offset, "&amp;", 5);
					offset += 5;
				}
				else
				{
					tmp[offset] = char(glyph & 0xff);
					offset += 1;
				}
			}
			else
			{
				int len = std::snprintf(tmp + offset, sizeof(tmp) - offset, "&#x%x;", glyph);
				offset += len;
			}

			if (offset >= 100)
			{
				real_append(std::string_view(tmp, offset));
				offset = 0;
			}
		}

		if (offset > 0)
			real_append(std::string_view(tmp, offset));
	}

	return *this;
}

GenericFormatter & GenericFormatter::operator<< (std::u32string const& s)
{
	return operator<< (std::u32string_view(s));
}

GenericFormatter & GenericFormatter::operator<< (Symbol s)
{
	return operator<< (s.to_string_view());
}
