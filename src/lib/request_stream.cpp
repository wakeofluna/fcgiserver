#include "request_stream.h"
#include <cassert>

using namespace fcgiserver;

RequestStream::RequestStream(Request & request, int (Request::*channel)(std::string_view const&), GenericFormat format)
    : GenericFormatter(format)
    , m_request(request)
    , m_channel(channel)
{
	assert(m_channel != nullptr && "invalid channel in RequestStream");
}

RequestStream & RequestStream::operator<< (HTMLContent const& value)
{
	if (m_generic_format == GenericFormat::HTML)
	{
		m_generic_format = GenericFormat::HTMLContent;
		GenericFormatter::operator<< (value.content);
		m_generic_format = GenericFormat::HTML;
	}
	else
	{
		GenericFormatter::operator<< (value.content);
	}
	return *this;
}

void RequestStream::real_append(std::string_view const& s)
{
	(m_request.*m_channel)(s);
}
