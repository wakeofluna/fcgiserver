#ifndef FCGISERVER_REQUEST_STREAM_H
#define FCGISERVER_REQUEST_STREAM_H

#include "fcgiserver_defs.h"
#include "generic_formatter.h"
#include <string_view>

namespace fcgiserver
{

class Request;

struct DLL_PUBLIC HTMLContent
{
	explicit constexpr HTMLContent(std::string_view s) : content(s) {}
	std::string_view content;
};

class DLL_PUBLIC RequestStream : public GenericFormatter
{
public:
	RequestStream(Request & request, int (Request::*channel)(std::string_view const&), GenericFormat format);
	RequestStream(Request const& other) = delete;
	RequestStream(Request && other) = delete;
	~RequestStream() = default;

	RequestStream & operator<< (HTMLContent const& value);

	template <typename T>
	inline RequestStream & operator<< (T const& value)
	{
		GenericFormatter::operator<<(value);
		return *this;
	}

protected:
	Request & m_request;
	int (Request::*m_channel)(std::string_view const& s);

	void real_append(std::string_view const& s) override;
};

}

#endif // FCGISERVER_REQUEST_STREAM_H
