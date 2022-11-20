#include "symbol.h"
#include "symbol_server.h"


namespace fcgiserver
{

Symbol::Symbol(const std::string & str)
{
	m_id = SymbolServer::instance().internalize(str);
}

Symbol::Symbol(const std::string_view & str)
{
	m_id = SymbolServer::instance().internalize(str);
}

Symbol::Symbol(const char * str)
{
	m_id = SymbolServer::instance().internalize(str);
}

Symbol Symbol::maybe(const std::string & str)
{
	return Symbol(SymbolServer::instance().check_internalization(str).second);
}

Symbol Symbol::maybe(const std::string_view & str)
{
	return Symbol(SymbolServer::instance().check_internalization(str).second);
}

Symbol Symbol::maybe(const char * str)
{
	return Symbol(SymbolServer::instance().check_internalization(str).second);
}

int Symbol::compare(const Symbol & other) const noexcept
{
	if (m_id == other.m_id) return 0;
	return to_string_view().compare(other.to_string_view());
}

std::string_view Symbol::to_string_view() const noexcept
{
	return SymbolServer::instance().resolve(m_id);
}

const char * Symbol::to_cstring() const noexcept
{
	std::string_view view = SymbolServer::instance().resolve(m_id);
	return view.empty() ? "" : view.data();
}

}

std::ostream & operator<< (std::ostream & stream, fcgiserver::Symbol const& s)
{
	stream << '[' << s.id() << ':' << s.to_string_view() << ']';
	return stream;
}
