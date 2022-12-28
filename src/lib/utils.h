#ifndef FCGISERVER_UTILS_H
#define FCGISERVER_UTILS_H

#include "fcgiserver_defs.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace fcgiserver
{
namespace utils
{

std::pair<bool,uint8_t> DLL_PUBLIC unhex(char const* ch);

struct convert_state
{
	std::uint32_t glyph = 0;
	std::uint8_t remaining = 0;
	bool valid = false;
	inline operator bool() const { return valid; }
};

char32_t DLL_PUBLIC utf8_to_32(std::uint8_t next, convert_state & state);
std::size_t DLL_PUBLIC utf32_to_8(char32_t glyph, std::uint8_t * buf);

} // namespace utils
} // namespace fcgiserver

#endif // UTILS_H
