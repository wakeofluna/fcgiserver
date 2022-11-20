#include "symbol_server.h"
#include <cassert>
#include <cstring>
#include <memory>


namespace fcgiserver
{

SymbolServer & SymbolServer::instance()
{
	static SymbolServer server;
	return server;
}

SymbolServer::~SymbolServer()
{
	clear();
}

unsigned int SymbolServer::internalize(std::string_view const& str)
{
	if (str.empty())
		return 0;

	std::shared_lock<std::shared_mutex> read_lock(m_mutex);

	auto found = m_index.find(str);
	if (found != m_index.end())
		return found->second;

	read_lock.unlock();

	// Prepare the new string
	std::size_t new_size = str.size();
	std::unique_ptr<char[]> new_str(new char[new_size + 1]);
	std::memcpy(new_str.get(), str.data(), new_size);
	new_str[new_size] = 0;
	std::string_view new_view(new_str.get(), new_size);

	// Lock the administration for writing
	std::lock_guard<std::shared_mutex> lock(m_mutex);

	// Recheck for data races :(
	found = m_index.find(str);
	if (found != m_index.end())
		return found->second;

	unsigned int new_index = static_cast<unsigned int>(m_strings.size());
	m_strings.emplace_back(new_str.get(), new_size);
	new_str.release();

	m_index[new_view] = new_index;
	return new_index;
}

std::string_view SymbolServer::resolve(unsigned int symbol_id) const
{
	std::shared_lock<std::shared_mutex> read_lock(m_mutex);

	assert(m_strings.size() > size_t(symbol_id));
	if (m_strings.size() > size_t(symbol_id))
	{
		auto & item = m_strings[symbol_id];
		return std::string_view(item.first, item.second);
	}
	else
	{
		return std::string_view("!!! invalid symbol !!!");
	}
}

SymbolServer::SymbolServer()
{
	m_strings.reserve(256);
	m_strings.emplace_back(nullptr, 0);
	m_index[std::string_view()] = 0;
}

void SymbolServer::clear()
{
	std::lock_guard<std::shared_mutex> lock(m_mutex);

	for (auto & item : m_strings)
		delete[] item.first;

	m_strings.clear();
	m_strings.emplace_back(nullptr, 0);
	m_index.clear();
	m_index[std::string_view()] = 0;
}

}
