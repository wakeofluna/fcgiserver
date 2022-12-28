#include "utils.h"

namespace fcgiserver
{
namespace utils
{

std::pair<bool,uint8_t> unhex(char const* ch)
{
	uint8_t v = 0;

	if (ch[0] >= '0' && ch[0] <= '9')
		v += (ch[0] - '0');
	else if (ch[0] >= 'a' && ch[0] <= 'f')
		v += (ch[0] - 'a' + 10);
	else if (ch[0] >= 'A' && ch[0] <= 'F')
		v += (ch[0] - 'A' + 10);
	else
		return {false, 0U};

	v <<= 4;

	if (ch[1] >= '0' && ch[1] <= '9')
		v += (ch[1] - '0');
	else if (ch[1] >= 'a' && ch[1] <= 'f')
		v += (ch[1] - 'a' + 10);
	else if (ch[1] >= 'A' && ch[1] <= 'F')
		v += (ch[1] - 'A' + 10);
	else
		return {false, 0U};

	return {true, v};
}

char32_t utf8_to_32(uint8_t next, convert_state & state)
{
	if (state.remaining > 0)
	{
		if (state.valid)
		{
			if ((next & 0xc0) == 0x80)
			{
				state.glyph <<= 6;
				state.glyph |= (next & 0x3f);
			}
			else
			{
				state.valid = false;
			}
		}
		--state.remaining;
	}
	else
	{
		if ((next & 0x80) == 0x00)
		{
			state.valid = true;
			state.glyph = next;
		}
		else if ((next & 0xe0) == 0xc0)
		{
			state.valid = true;
			state.remaining = 1;
			state.glyph = (next & 0x1f);
		}
		else if ((next & 0xf0) == 0xe0)
		{
			state.valid = true;
			state.remaining = 2;
			state.glyph = (next & 0x0f);
		}
		else if ((next & 0xf8) == 0xf0)
		{
			state.valid = true;
			state.remaining = 3;
			state.glyph = (next & 0x07);
		}
		else
		{
			state.valid = false;
		}
	}

	if (state.remaining > 0)
		return 0;
	else if (state.valid)
		return state.glyph;
	else
		return 65533; // replacement character
}

size_t utf32_to_8(char32_t glyph, uint8_t * buf)
{
	if (glyph < 0x80)
	{
		buf[0] = glyph;
		return 1;
	}
	else if (glyph < 0x800)
	{
		buf[0] = 0xc0 + (glyph >> 6);
		buf[1] = 0x80 + (glyph & 0x3f);
		return 2;
	}
	else if (glyph < 0x10000)
	{
		buf[0] = 0xe0 + (glyph >> 12);
		buf[1] = 0x80 + ((glyph >> 6) & 0x3f);
		buf[2] = 0x80 + (glyph & 0x3f);
		return 3;
	}
	else if (glyph < 0x110000)
	{
		buf[0] = 0xf0 + (glyph >> 18);
		buf[1] = 0x80 + ((glyph >> 12) & 0x3f);
		buf[2] = 0x80 + ((glyph >> 6) & 0x3f);
		buf[3] = 0x80 + (glyph & 0x3f);
		return 4;
	}
	else
	{
		return 0;
	}
}

} // namespace utils
} // namespace fcgiserver
