#ifndef FCGISERVER_GENERIC_FORMATTER_H
#define FCGISERVER_GENERIC_FORMATTER_H

#include "fcgiserver_defs.h"
#include "symbol.h"
#include <cstdarg>
#include <string_view>


namespace fcgiserver
{

enum class GenericFormat : char
{
	/// Byte for byte passthrough, no encoding
	Verbatim,

	/// Encode multibyte glyphs as html entities and encode the character '&'
	HTML,

	/// Encode multibyte glyphs as html entities and encode the characters '<', '>' and '&'
	HTMLContent,

	/// Encode multibyte glyphs as UTF8
	UTF8,
};

class DLL_PUBLIC GenericFormatter
{
public:
	GenericFormatter(GenericFormat format = GenericFormat::Verbatim);
	virtual ~GenericFormatter() = default;

	template <typename T>
	inline GenericFormatter & append(T arg)
	{
		operator<<(std::forward<T>(arg));
		return *this;
	}

	template <typename T, typename ...ARGS>
	inline GenericFormatter & append(T arg, ARGS... args)
	{
		operator<<(std::forward<T>(arg));
		return append(std::forward<ARGS>(args)...);
	}

	GenericFormatter & printf(char const* fmt, ...);
	GenericFormatter & vprintf(char const* fmt, std::va_list vl);

	GenericFormatter & operator<< (bool b);
	GenericFormatter & operator<< (char c);
	GenericFormatter & operator<< (uint8_t i);
	GenericFormatter & operator<< (int8_t i);
	GenericFormatter & operator<< (uint16_t i);
	GenericFormatter & operator<< (int16_t i);
	GenericFormatter & operator<< (uint32_t i);
	GenericFormatter & operator<< (int32_t i);
	GenericFormatter & operator<< (uint64_t i);
	GenericFormatter & operator<< (int64_t i);
	GenericFormatter & operator<< (float f);
	GenericFormatter & operator<< (double d);
	GenericFormatter & operator<< (const char * s);
	GenericFormatter & operator<< (std::string_view const& s);
	GenericFormatter & operator<< (std::string const& s);
	GenericFormatter & operator<< (std::u32string_view const& s);
	GenericFormatter & operator<< (std::u32string const& s);
	GenericFormatter & operator<< (Symbol s);

	inline GenericFormat generic_format() const { return m_generic_format; }
	inline void set_generic_format(GenericFormat format) { m_generic_format = format; }

protected:
	GenericFormat m_generic_format;
	virtual void real_append(std::string_view const& s) = 0;
};

}

#endif // FCGISERVER_GENERIC_FORMATTER_H
