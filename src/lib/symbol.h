#ifndef FCGISERVER_SYMBOL_H
#define FCGISERVER_SYMBOL_H

#include "fcgiserver_defs.h"
#include <string_view>
#include <string>
#include <functional>
#include <ostream>

namespace fcgiserver
{

class SymbolServer;

class DLL_PUBLIC Symbol
{
public:
	inline Symbol() noexcept : m_id(0) {}
	Symbol(std::string const& str);
	Symbol(std::string_view const& str);
	Symbol(char const* str);

	static Symbol maybe(std::string const& str);
	static Symbol maybe(std::string_view const& str);
	static Symbol maybe(char const* str);

	inline Symbol(Symbol const& other) noexcept : m_id(other.m_id) {}
	inline Symbol(Symbol && other) noexcept : m_id(other.m_id) { other.m_id = 0; }
	inline Symbol& operator= (Symbol const& other) noexcept { m_id = other.m_id; return *this; }
	inline Symbol& operator= (Symbol && other) noexcept { m_id = other.m_id; other.m_id = 0; return *this; }
	inline Symbol& operator= (std::string const& str) { *this = Symbol(str); return *this; }
	inline Symbol& operator= (std::string_view const& str) { *this = Symbol(str); return *this; }
	inline Symbol& operator= (char const* str) { *this = Symbol(str); return *this; }

	inline unsigned int id() const noexcept { return m_id; }

	int compare(Symbol const& other) const noexcept;

	std::string_view to_string_view() const noexcept;
	char const* to_cstring() const noexcept;
	inline operator std::string_view() const noexcept { return to_string_view(); }
	inline operator char const*() const noexcept { return to_cstring(); }

	inline operator bool() const noexcept { return m_id != 0; }
	inline bool operator== (Symbol const& other) const noexcept { return m_id == other.m_id; }
	inline bool operator!= (Symbol const& other) const noexcept { return m_id != other.m_id; }
	inline bool operator< (Symbol const& other) const noexcept { return compare(other) < 0; }
	inline bool operator<= (Symbol const& other) const noexcept { return compare(other) <= 0; }
	inline bool operator> (Symbol const& other) const noexcept { return compare(other) > 0; }
	inline bool operator>= (Symbol const& other) const noexcept { return compare(other) >= 0; }

private:
	inline explicit Symbol(unsigned int id) : m_id(id) {}
	unsigned int m_id;
};

}


DLL_PUBLIC std::ostream & operator<< (std::ostream & stream, fcgiserver::Symbol const& s);

template <>
struct std::hash<fcgiserver::Symbol>
{
	inline std::size_t operator()(fcgiserver::Symbol const& s) const noexcept
	{
		return s.id();
	}
};


#endif // FCGISERVER_SYMBOL_H
