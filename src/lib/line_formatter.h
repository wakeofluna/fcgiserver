#ifndef FCGISERVER_LINEFORMATTER_H
#define FCGISERVER_LINEFORMATTER_H

#include <cstdarg>
#include <cstdint>
#include <string>
#include <string_view>
#include "fcgiserver_defs.h"
#include "symbol.h"

namespace fcgiserver
{

class DLL_PUBLIC LineFormatter
{
public:
	LineFormatter();
	LineFormatter(std::string && buffer);
	LineFormatter(LineFormatter const& other) = delete;
	LineFormatter(LineFormatter && other) = delete;
	LineFormatter & operator= (LineFormatter const& other) = delete;
	LineFormatter & operator= (LineFormatter && other) = delete;
	~LineFormatter();

	inline std::string & buffer() { return m_buffer; }
	inline std::string const& buffer() const { return m_buffer; }

	inline operator std::string_view() const { return m_buffer; }

	void clear();
	bool empty() const;

	template <typename T>
	inline LineFormatter & append(T arg)
	{
		operator<<(std::forward<T>(arg));
		return *this;
	}

	template <typename T, typename ...ARGS>
	inline LineFormatter & append(T arg, ARGS... args)
	{
		operator<<(std::forward<T>(arg));
		return append(std::forward<ARGS>(args)...);
	}

	LineFormatter & printf(char const* fmt, ...);
	LineFormatter & vprintf(char const* fmt, std::va_list vl);

	LineFormatter & operator<< (bool b);
	LineFormatter & operator<< (char c);
	LineFormatter & operator<< (uint8_t i);
	LineFormatter & operator<< (int8_t i);
	LineFormatter & operator<< (uint16_t i);
	LineFormatter & operator<< (int16_t i);
	LineFormatter & operator<< (uint32_t i);
	LineFormatter & operator<< (int32_t i);
	LineFormatter & operator<< (uint64_t i);
	LineFormatter & operator<< (int64_t i);
	LineFormatter & operator<< (float f);
	LineFormatter & operator<< (double d);
	LineFormatter & operator<< (const char * s);
	LineFormatter & operator<< (std::string_view const& s);
	LineFormatter & operator<< (std::string const& s);
	LineFormatter & operator<< (Symbol s);

protected:
	std::string m_buffer;
};

}

#endif // LINEFORMATTER_H
