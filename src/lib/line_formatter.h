#ifndef FCGISERVER_LINEFORMATTER_H
#define FCGISERVER_LINEFORMATTER_H

#include <cstdarg>
#include <cstdint>
#include <string>
#include <string_view>
#include "fcgiserver_defs.h"
#include "generic_formatter.h"

namespace fcgiserver
{

class DLL_PUBLIC LineFormatter : public GenericFormatter
{
public:
	LineFormatter(GenericFormat format = GenericFormat::UTF8);
	LineFormatter(std::string && buffer, GenericFormat format = GenericFormat::UTF8);
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

protected:
	void real_append(const std::string_view & s) override;

	std::string m_buffer;
};

}

#endif // LINEFORMATTER_H
