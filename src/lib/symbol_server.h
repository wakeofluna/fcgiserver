#ifndef FCGISERVER_SYMBOLSERVER_H
#define FCGISERVER_SYMBOLSERVER_H

#include "fcgiserver_defs.h"
#include <unordered_map>
#include <shared_mutex>
#include <string_view>
#include <utility>
#include <vector>

namespace fcgiserver
{

class DLL_PRIVATE SymbolServer
{
public:
	static SymbolServer & instance();
	~SymbolServer();

	std::pair<bool,unsigned int> check_internalization(std::string_view const& str) const;
	unsigned int internalize(std::string_view const& str);
	inline unsigned int internalize(std::string const& str) { return internalize(std::string_view(str)); }
	inline unsigned int internalize(char const* str) { return internalize(std::string_view(str)); }

	std::string_view resolve(unsigned int symbol_id) const;

private:
	SymbolServer();
	void clear();
	void insert_defaults();

	mutable std::shared_mutex m_mutex;
	std::vector<std::pair<char*,size_t>> m_strings;
	std::unordered_map<std::string_view,unsigned int> m_index;
};

}

#endif // FCGISERVER_SYMBOLSERVER_H
